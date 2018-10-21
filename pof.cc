// -*- mode: c++; -*-

#define BOOST_LOG_DYN_LINK 1

#include <cctype>
#include <cstring>
#include <cmath>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

namespace fs = std::filesystem;

#include <GL/gl.h>

#ifdef NDEBUG
#  define BOOST_DISABLE_ASSERTS
#  define ASSERT(x) x
#else
#  define ASSERT BOOST_ASSERT
#endif // NDEBUG

#include <boost/assert.hpp>
#include <boost/endian/arithmetic.hpp>
using namespace boost::endian;

#include "algorithm.hh"
#include "log.hh"
#include "pof.hh"
#include "stream.hh"
#include "util.hh"
#include "vector.hh"

static int file_size = 0;
static int file_version = 0;

////////////////////////////////////////////////////////////////////////

template< typename T >
inline const T& ref (const char& c) {
    return reinterpret_cast< const T& > (c);
}

#define ref_i(x)   ref< int > (x)
#define ref_u(x)   ref< unsigned > (x)
#define ref_s(x)   ref< short > (x)
#define ref_us(x)  ref< unsigned short > (x)
#define ref_f(x)   ref< float > (x)
#define ref_v3f(x) ref< vector3f_t > (x)
#define ref_v3i(x) ref< vector3i_t > (x)

struct bsp_t {
    vector< vector3f_t > vertices, normals;
    vector< pof_t::poly_t > polys;
    vector< pof_t::box_t > boxes;
};

static void
read (const char* p, bsp_t& bsp) {
    while (ref_i (p [0])) {

        int id = ref_i (p [0]);
        int size = ref_i (p [4]);

        switch (id) {
        case POINT_DEF: {
            int n = ref_i (p [8]);  // vertices

            const char* s = p;
            s += ref_i (p [16]);

            vector< int > normal_counts (size_t (n), { });

            for (int i = 0; i < n; ++i)
                normal_counts [i] = p [20 + i];

            for (int i = 0; i < n; ++i) {
                //
                // Add vertex and set its subobject:
                //
                bsp.vertices.push_back (ref_v3f (s [0]));
                s += 12;

                //
                // For each vertex, store a set of normals:
                //
                for (int j = 0; j < normal_counts [i]; ++j) {
                    bsp.normals.push_back (ref_v3f (s [0]));
                    s += 12;
                }
            }
        }
            break;

        case FLATPOLY_DEF: {
            bsp.polys.push_back ({ });
            auto& poly = bsp.polys.back ();

            poly.type = FLATPOLY_DEF;

            poly.normal = ref_v3f (p [8]);
            poly.center = ref_v3f (p [20]);
            poly.radius = ref_f   (p [32]);

            int n = ref_i (p [36]); // number of vertices

            poly.vertices.resize (n, { });
            poly.normals. resize (n, { });

            poly.color = ref_i (p [40]);

            for (int i = 0; i < n; ++i) {
                poly.vertices [i] = ref_s (p [44 + (i * 4)]);
                poly.normals  [i] = ref_s (p [46 + (i * 4)]);
            }
        }
            break;

        case TEXTPOLY_DEF: {
            bsp.polys.push_back ({ });
            auto& poly = bsp.polys.back ();

            poly.type = TEXTPOLY_DEF;

            poly.normal = ref_v3f (p [8]);
            poly.center = ref_v3f (p [20]);
            poly.radius = ref_f   (p [32]);

            int n = ref_i (p [36]); // number of vertices

            poly.vertices.resize (n, { });
            poly.normals. resize (n, { });

            poly.u.resize (n, { });
            poly.v.resize (n, { });

            //
            // For textured polygons this is a texture map index
            //
            poly.color = ref_i (p [40]);

            for (int i = 0; i < n; ++i) {
                poly.vertices [i] = ref_s (p [44 + (i * 12)]);
                poly.normals  [i] = ref_s (p [46 + (i * 12)]);

                poly.u [i] = ref_f (p [48 + (i * 12)]);
                poly.v [i] = ref_f (p [52 + (i * 12)]);
            }
        }
            break;

        case BSP_DEF: {
            read (p + ref_i (p [36]), bsp);
            read (p + ref_i (p [40]), bsp);
        }
            break;

        case BOX_DEF: {
            bsp.boxes.push_back ({ ref_v3f (p [8]), ref_v3f (p [20]) });
        }
            break;

        default:
            ASSERT (0);
            break;
        }

        p += size;
    }
}

static void
postprocess (pof_t& pof) {
    //
    // Reset all subobjects detail:
    //
    for (size_t i = 0; i < pof.subobjs.size (); ++i)
        pof.subobjs [i].detail = -1;

    {
        //
        // Set subobject's detail (6 details, each index points to a subobject for
        // that detail level):
        //
        int detail = 0;

        for (auto i : pof.detail_subobj)
            pof.subobjs [i].detail = detail++;
    }

    //
    // Each debris subobject gets a detail level of 9:
    //
    for (auto i : pof.debris_subobj)
        pof.subobjs [i].detail = 9;

    for (size_t i = 0; i < pof.subobjs.size (); ++i) {
        if (0 <= pof.subobjs [i].parent) {
            //
            // For each child object ...:
            //
            int j = i;

            //
            // ... traverse to root subobject ...:
            //
            while (0 <= pof.subobjs [j].parent)
                j = pof.subobjs [j].parent;

            //
            // ... and borrow the detail level from root:
            //
            ASSERT (0 > pof.subobjs [j].parent);
            pof.subobjs [i].detail = pof.subobjs [j].detail;
        }
    }
}

static void
postprocess (pof_t& pof, bsp_t& bsp) {
    auto& subobj = pof.subobjs.back ();

    int base_subobj = pof.subobjs.size () - 1;
    int base_vertex = pof.vertices.size ();

    for (auto& v : bsp.vertices)
        v += subobj.off;

    pof.vertices.insert (
        pof.vertices.end (),
        bsp.vertices.begin (), bsp.vertices.end ());

    pof.subobj_indices.resize (
        pof.subobj_indices.size () + bsp.vertices.size (),
        base_subobj);

    pof.normals.insert (
        pof.normals.end (),
        bsp.normals.begin (), bsp.normals.end ());

    for (auto& p : bsp.polys) {
        p.subobj_index = base_subobj;

        for (auto& v : p.vertices)
            v += base_vertex;
    }

    pof.polys.insert (
        pof.polys.end (),
        bsp.polys.begin (), bsp.polys.end ());
}

static istream&
read (istream& s, pof_t& pof) {
    s.seekg (0, ios_base::end);

    file_size = s.tellg ();
    s.seekg (0, ios_base::beg);

    II << " --> file size : " << file_size;

    int file_id = 0;
    ASSERT (read (s, file_id));

    big_to_native_inplace (file_id);
    ASSERT (file_id == 'PSPO');

    ASSERT (read (s, file_version));

    II << " --> file_id : " << string_from (file_id) << ", file version : "
       << hex << file_version;

    while (s.tellg () < file_size) {
        int id{ };
        ASSERT (read (s, id));

        big_to_native_inplace (id);

        int len = 0;
        ASSERT (read (s, len));

        II << "  --> id : " << hex << string_from (id) << " ("
           << dec << len << ")";

        streamoff fpos = s.tellg ();
        streamoff next = fpos + len;

        ASSERT (next <= file_size);

        switch (id) {
        case 'OHDR':
            ASSERT (0);

        case 'HDR2': {
            ASSERT (read (s, pof.radius));
            ASSERT (read (s, pof.flags));

            {
                int n{ };
                ASSERT (read (s, n));

                II << "    --> sub-objects : " << n;
            }

            ASSERT (read (s, pof.minbox));
            ASSERT (read (s, pof.maxbox));

            {
                int n{ };
                ASSERT (read (s, n));

                II << "    --> details : " << n;

                if (n) {
                    pof.detail_subobj.resize (size_t (n), { });

                    for (auto& detail : pof.detail_subobj)
                        ASSERT (read (s, detail));
                }
            }

            {
                int n{ };
                ASSERT (read (s, n));

                II << "    --> debris : " << n;

                if (n) {
                    pof.debris_subobj.resize (size_t (n), { });

                    for (auto& debris : pof.debris_subobj)
                        ASSERT (read (s, debris));
                }
            }

            if (file_version >= 1903) {
                float scale = 1.0f;

                ASSERT (read (s, pof.mass));
                ASSERT (read (s, pof.mass_center));

                if (file_version < 2009) {
                    double v = pof.mass;
                    double a = 4.65 * pow (pof.mass, 2 / 3);
                    scale = float (v / a);
                    pof.mass = a;
                }

                for (int j = 0; j < 3; ++j) {
                    for (int i = 0; i < 3; ++i) {
                        float value;
                        ASSERT (read (s, value));
                        pof.inertia_tensor [i][j] = value * scale;
                    }
                }
            }
            else {
                pof.mass = 1.0;

                pof.mass_center.value [0] = 0.0;
                pof.mass_center.value [1] = 0.0;
                pof.mass_center.value [2] = 0.0;

                auto& x = pof.inertia_tensor;
                fill (&x[0][0], &x[0][0] + sizeof x / sizeof **x, 0);
            }

            II << "    --> mass : " << pof.mass;
            II << "    --> mass center : " << pof.mass_center;

            if (file_version >= 2014) {
                int n{ };
                ASSERT (read (s, n));

                II << "    --> cross-sections : " << n;

                if (0 < n) {
                    pof.cross_sections.resize (size_t (n), { });

                    for (auto& x : pof.cross_sections) {
                        ASSERT (read (s, x.depth));
                        ASSERT (read (s, x.radius));
                    }
                }
            }

            if (file_version >= 2007) {
                int n{ };

                ASSERT (read (s, n));
                ASSERT (n != 'SOBJ' && n != 'OBJ2');

                pof.lights.resize (size_t (n), { });

                II << "    --> lights : " << n;

                for (auto& light : pof.lights) {
                    ASSERT (read (s, light.pos));
                    ASSERT (read (s, light.type));
                    ASSERT (1 == light.type || 2 == light.type);
                }
            }
        }
            break;

        case 'TXTR': {
            int n{ };
            ASSERT (read (s, n));

            pof.textures.resize (size_t (n), { });

            for (auto& text : pof.textures) {
                ASSERT (read (s, text.name));
                II << "    --> " << text.name;
            }
        }
            break;

        case 'SHLD': {
            {
                int n{ };
                ASSERT (read (s, n));

                pof.shield.vertices.resize (size_t (n), { });

                for (auto& v : pof.shield.vertices)
                    ASSERT (read (s, v));
            }

            {
                int n{ };
                ASSERT (read (s, n));

                pof.shield.faces.resize (size_t (n), { });

                for (auto& f : pof.shield.faces) {
                    ASSERT (read (s, f.normal));

                    ASSERT (read (s, f.vertices [0]));
                    ASSERT (read (s, f.vertices [1]));
                    ASSERT (read (s, f.vertices [2]));

                    ASSERT (read (s, f.neighbors [0]));
                    ASSERT (read (s, f.neighbors [1]));
                    ASSERT (read (s, f.neighbors [2]));
                }
            }
        }
            break;

        case 'SOBJ':
        case 'OBJ2': {
            pof.subobjs.push_back ({ });
            auto& subobj = pof.subobjs.back ();

            ASSERT (read (s, subobj.number));
            II << "    --> sub-object : " << subobj.number;

            if (id == 'OBJ2')
                ASSERT (read (s, subobj.radius));

            ASSERT (read (s, subobj.parent));   // parent
            ASSERT (read (s, subobj.real_off)); // offset

            subobj.off = subobj.real_off;

            if (subobj.parent != -1)
                subobj.off += pof.subobjs [subobj.parent].off;

            if (id == 'SOBJ')
                ASSERT (read (s, subobj.radius)); //rad

            ASSERT (read (s, subobj.center));
            ASSERT (read (s, subobj.minbox));
            ASSERT (read (s, subobj.maxbox));

            ASSERT (read (s, subobj.name));
            ASSERT (read (s, subobj.properties));

            II << "    --> name : " << subobj.name;
            II << "    --> prop : " << subobj.properties;

            ASSERT (read (s, subobj.movement.type));
            ASSERT (read (s, subobj.movement.axis));

            int ignore;

            ASSERT (read (s, ignore));
            ASSERT (0 == ignore);

            int n = 0;
            ASSERT (read (s, n));

            II << "    --> BSP data : " << n << " bytes";

            vector< char > arr (size_t (n), 0);
            ASSERT (read (s, arr));

            bsp_t bsp{ };
            read (arr.data (), bsp);

            postprocess (pof, bsp);
        }
            break;

        case 'GPNT':
        case 'MPNT': {
            int guntype = id == 'GPNT' ? 0 : 1;

            auto& guns = pof.guns[guntype];

            int n{ };
            ASSERT (read (s, n));

            guns.resize (size_t (n), { });

            for (int i = 0; i < n; ++i) {
                auto& slot = guns [i];

                int n{ };
                ASSERT (read (s, n));

                slot.resize (size_t (n), { });

                for (int j = 0; j < n; ++j) {
                    //
                    // One normal per gun:
                    //
                    ASSERT (read (s, slot [j].pos));
                    ASSERT (read (s, slot [j].normal));

                    //
                    // Store the guns defined here in the global gun directory:
                    //
                    pof.weapons.push_back ({ });
                    auto& weapon = pof.weapons.back ();

                    weapon.type = id == 'GPNT' ? GUN_TYPE : MISSILE_TYPE;

                    weapon.pos = slot [j].pos;
                    weapon.normal = slot [j].normal;

                    weapon.subobj = 0;
                    weapon.bank = i;
                }
            }
        }
            break;

        case 'TGUN':
        case 'TMIS': {
            int guntype = id == 'TGUN' ? 0 : 1;

            auto& turret_banks = pof.turret_banks [guntype];

            int n{ };
            ASSERT (read (s, n));

            turret_banks.resize (size_t (n), { });

            for (int i = 0; i < n; ++i) {
                auto& bank = turret_banks [i];

                ASSERT (read (s, bank.barrel_subobj));
                ASSERT (read (s, bank.mount_subobj));

                //
                // One normal, only:
                //
                ASSERT (read (s, bank.normal));

                int n{ };
                ASSERT (read (s, n));

                bank.pos.resize (size_t (n), { });

                for (int j = 0; j < n; ++j) {
                    ASSERT (read (s, bank.pos [j]));

                    //
                    // Store the guns defined here in the global gun directory:
                    //
                    pof.weapons.push_back ({ });
                    auto& weapon = pof.weapons.back ();

                    weapon.type = id == 'TGUN'
                        ? GUN_TURRET_TYPE : MISSILE_TURRET_TYPE;

                    weapon.pos = bank.pos [j];
                    weapon.normal = bank.normal;

                    weapon.subobj = 0;
                    weapon.bank = i;
                }
            }
        }
            break;

        case 'SPCL': {
            int n{ };
            ASSERT (read (s, n));

            pof.subsys.resize (size_t (n), { });

            for (int i = 0; i < n; ++i) {
                auto& subsys = pof.subsys [i];

                ASSERT (read (s, subsys.name));
                ASSERT (read (s, subsys.properties));
                ASSERT (read (s, subsys.pos));
                ASSERT (read (s, subsys.radius));
            }
        }
            break;

        case 'DOCK': {
            int n{ };
            ASSERT (read (s, n));

            pof.docks.resize (size_t (n), { });

            for (int i = 0; i < n; ++i) {
                auto& dock = pof.docks [i];

                ASSERT (read (s, dock.properties));

                {
                    int n{ };
                    ASSERT (read (s, n));

                    dock.splines.resize (size_t (n), { });

                    for (int j = 0; j < n; ++j)
                        ASSERT (read (s, dock.splines [j]));
                }

                {
                    int n{ };
                    ASSERT (read (s, n));

                    dock.pos.resize (size_t (n), { });
                    dock.normal.resize (size_t (n), { });

                    for (int j = 0; j < n; j++) {
                        ASSERT (read (s, dock.pos [j]));
                        ASSERT (read (s, dock.normal [j]));
                    }
                }
            }
        }
            break;

        case 'PATH': {
            //
            // Ignored
            //
            int num_paths = 0;
            ASSERT (read (s, num_paths));

            for (int i = 0; i < num_paths; ++i) {
                string y, z;

                ASSERT (read (s, y));
                ASSERT (read (s, z));

                int num_verts = 0;
                ASSERT (read (s, num_verts));

                for (int j = 0; j < num_verts; j++) {
                    {
                        vector3f_t ignore;
                        ASSERT (read (s, ignore));
                    }

                    {
                        float ignore;
                        ASSERT (read (s, ignore));
                    }

                    int ignore = 0;
                    ASSERT (read (s, ignore));

                    for (int k = 0; k < ignore; k++) {
                        int ignore = 0;
                        ASSERT (read (s, ignore));
                    }
                }
            }
        }
            break;

        case 'FUEL': {
            int n{ };
            ASSERT (read (s, n));

            II << "    --> thrusters : " << n;
            pof.thrusters.resize (size_t (n), { });

            for (int i = 0; i < n; ++i) {
                auto& thruster = pof.thrusters [i];

                int n{ };
                ASSERT (read (s, n));

                II << "      --> glows : " << n;
                thruster.glows.resize (size_t (n), { });

                if (file_version >= 2117)
                    ASSERT (read (s, thruster.properties));

                for (int j = 0; j < n; ++j) {
                    auto& glow = thruster.glows[j];

                    ASSERT (read (s, glow.pos));
                    ASSERT (read (s, glow.normal));
                    ASSERT (read (s, glow.radius));
                }
            }
        }
            break;

        case 'PINF': {
            vector< char > buf (size_t (next - s.tellg ()), 0);

            if (!buf.empty ())
                ASSERT (read (s, buf));

            string text;

            for (auto c : buf) {
                if (0 == c)
                    c = '\n';

                text += c;
            }

            II << "    --> Compilation data : \n\"" << text << "\"";
        }
            break;

        case 'EYE ': {
            int n{ };

            ASSERT (read (s, n));
            ASSERT (0 == n || 1 == n);

            II << "    --> eyes : " << n;

            if (n) {
                pof.eyes.resize (size_t (n), { });

                ASSERT (read (s, pof.eyes [0].subobj_index));
                // ASSERT (pof.eyes [0].subobj_index < pof.nsubobjs);

                II << "    --> subobj : " << pof.eyes [0].subobj_index;

                ASSERT (read (s, pof.eyes [0].off));
                ASSERT (read (s, pof.eyes [0].normal));
            }
        }

            break;

        case 'INSG': {
            int tmp = 0;
            ASSERT (read (s, tmp));

            s.seekg (-4, ios_base::cur);
        }
            break;

        case 'ACEN':
            ASSERT (len == 12);
            ASSERT (read (s, pof.autocenter_point));

            break;

        default:
            ASSERT (0);
            break;
        }

        streamoff off = s.tellg ();
        ASSERT (off <= next);

        if (off < next) {
            WW << "offset : " << off << " < " << next;
            s.seekg (next, ios_base::beg);
        }
    }

    return postprocess (pof), s;
}

////////////////////////////////////////////////////////////////////////

int main (int, char** argv) {
    ASSERT (argv [1] && argv [1][0]);

    if (fs::exists (argv [1]) && fs::is_regular_file (argv [1])) {
        file_size = fs::file_size (argv [1]);

        ifstream s (argv [1], ios_base::in | ios_base::binary);
        s.exceptions (ios_base::badbit | ios_base::failbit);

        auto p = make_unique< pof_t > ();
        ASSERT (read (s, *p));

        return 0;
    }
    else {
        EE << "missing file : " << argv [1];
        return 1;
    }
}
