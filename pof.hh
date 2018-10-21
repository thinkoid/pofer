// -*- mode: c++; -*-

#ifndef POF_POF_HH
#define POF_POF_HH

#include "vector.hh"

////////////////////////////////////////////////////////////////////////

#define POINT_DEF     1 // points definition
#define FLATPOLY_DEF  2 // flat-shaded polygon definition
#define TEXTPOLY_DEF  3 // texture-mapped polygon definition
#define BSP_DEF       4 // BSP, sort by normal
#define BOX_DEF       5 // bounding box definition

#define GUN_TYPE             1
#define MISSILE_TYPE         2
#define GUN_TURRET_TYPE      3
#define MISSILE_TURRET_TYPE  4

#define MAX_BLOCKS           8192
#define MAX_BOXES            320000
#define MAX_CROSSSECTIONS    256
#define MAX_LIGHTS           256

#define MAX_DOCKS            1024
#define MAX_DOCKING_POINTS   4096
#define MAX_SPLINE_PATHS     256

#define MAX_SHIELD_FACES     48000
#define MAX_SHIELD_VERTICES  24000

#define MAX_SUBOBJ_POINTS  30000
#define MAX_SUBOBJS          200

#define MAX_SUBSYSTEMS       1024
#define MAX_THRUSTERS        32
#define MAX_THRUSTER_GLOWS   32

#define MAX_VERTICES         150000
#define MAX_NORMALS          320000
#define MAX_POLYGONS         160000
#define MAX_TEXTURES         60

#define MAX_WEAPONS          256
#define MAX_GUN_SLOTS        128
#define MAX_SLOT_CAPACITY    128
#define MAX_TURRET_BANKS     128
#define MAX_BANK_CAPACITY    128

////////////////////////////////////////////////////////////////////////

struct pof_t {
    int flags;

    vector3f_t minbox, maxbox;
    float radius;

    struct box_t {
        vector3f_t minbox, maxbox;
    };

    vector< box_t > boxes;

    float mass;
    vector3f_t mass_center;
    float inertia_tensor [3][3];

    vector3f_t autocenter_point;

    struct cross_section_t {
        float depth, radius;
    };

    vector< cross_section_t > cross_sections;

    struct light_t {
        vector3f_t pos;
        int type;
    };

    vector< light_t > lights;

    struct eye_t {
        vector3f_t off, normal;
        int subobj_index;
    };

    vector< eye_t > eyes;

    struct subobj_t {
        int number, parent, detail;
        string name, properties;

        vector3f_t center, off, real_off;

        vector3f_t minbox, maxbox;
        float radius;

        struct {
            int type, axis;
        } movement;
    };

    vector< subobj_t > subobjs;
    vector< int > detail_subobj, debris_subobj;

    vector< vector3f_t > vertices, normals;
    vector< int > subobj_indices;

    struct poly_t {
        int type, color, subobj_index;
        vector3f_t center, normal;
        float radius;
        vector< int > vertices, normals;
        vector< float > u, v;
    };

    vector< poly_t > polys;

    struct texture_t {
        string name;
        size_t x, y, x2, y2;
        int detail;
    };

    vector< texture_t > textures;

    struct shield_t {
        vector< vertex3f_t > vertices;

        struct face_t {
            vector3f_t normal;
            int vertices [3];
            int neighbors [3];
        };
        vector< face_t > faces;
    };

    shield_t shield;

    struct thruster_t {
        string properties;

        struct glow_t {
            vector3f_t pos, normal;
            float radius;
        };

        vector< glow_t > glows;
    };

    vector< thruster_t > thrusters;

    struct dock_t {
        string properties;
        vector< int > splines;
        vector< vector3f_t > pos, normal;
    };

    vector< dock_t > docks;

    struct subsys_t {
        string name, properties;
        vertex3f_t pos;
        float radius;
    };

    vector< subsys_t > subsys;

    struct weapon_t {
        vector3f_t pos, normal;
        int subobj, type, bank;
    };

    vector< weapon_t > weapons;

    struct gun_t {
        vector3f_t pos, normal;
    };

    vector< vector< gun_t > > guns [2];

    struct turret_bank_t {
        int barrel_subobj, mount_subobj;
        vector< vector3f_t > pos; // firing positions
        vector3f_t normal;
    };

    vector< turret_bank_t > turret_banks [2];
};

#endif // POF_POF_HH
