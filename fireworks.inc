// built-in include guard removal
// just in case the user has a local dependency with the same file name
#if defined _inc_fireworks
    #undef _inc_fireworks
#endif
// custom include-guard to ensure we don't duplicate
#if defined _fireworks_included
    #endinput
#endif
#define _fireworks_included

#include <a_samp>
#include <logger>
#include <streamer>
#include <YSI\y_iterate>
#include <YSI\y_hooks>


#if !defined MAX_FIREWORK_EXPLOSION_TYPES
    #define MAX_FIREWORK_EXPLOSION_TYPES (FireworkExplosion:64)
#endif

#if !defined MAX_FIREWORK_SEQUENCES
    #define MAX_FIREWORK_SEQUENCES (FireworkSequence:64)
#endif

#if !defined MAX_EXPLOSION_MODELS
    #define MAX_EXPLOSION_MODELS (12)
#endif

#if !defined MAX_PROJECTILE
    #define MAX_PROJECTILE (4096)
#endif

#if !defined MAX_SEQUENCE_EXPLOSIONS
    #define MAX_SEQUENCE_EXPLOSIONS (16)
#endif

#if !defined FIREWORK_STREAMER_IDENTIFIER
    #define FIREWORK_STREAMER_IDENTIFIER (8275672)
#endif

#define INVALID_EXPLOSION_TYPE (FireworkExplosion:-1)
#define INVALID_SEQUENCE (FireworkSequence:-1)
#define INVALID_SPREAD_COUNT (-1)


forward FireworkExplosion:DefineFireworkExplosionType(models[], totalModels, Float:elevation, Float:distance, spread);
forward FireworkSequence:DefineFireworkExplosionSequence(FireworkExplosion:...);
forward CreateFirework(FireworkSequence:sequence, Float:x, Float:y, Float:z);
forward bool:IsValidFireworkExplosionID(FireworkExplosion:id);
forward bool:IsValidFireworkSequence(FireworkSequence:id);


// E_EXPLOSION_TYPE_DATA represents a style of explosion, it consists of a set of models to create
// the explosion from, the elevation angle to move the objects towards and the distance the objects
// will move as well as a spread value which determines how many objects are created.
enum E_EXPLOSION_TYPE_DATA {
    fwk_model[MAX_EXPLOSION_MODELS],
    fwk_models,
    Float:fwk_elevation,
    Float:fwk_distance,
    fwk_spread
}

// E_FIREWORK_PROJECTILE_DATA represents an individual projectile that's currently in-flight. These
// are created as a result of an "explosion" being triggered and stores information for the next
// explosion type to trigger once this has finished.
enum E_FIREWORK_PROJECTILE_DATA {
    fwk_object,                    // the object ID this projectile is using
    FireworkSequence:fwk_sequence, // the sequence ID this projectile is part of
    fwk_index                      // the index through the sequence
}

// E_SEQUENCE_DATA represents a sequence of explosions.
enum E_SEQUENCE_DATA {
    FireworkExplosion:fwk_sequenceExp[MAX_SEQUENCE_EXPLOSIONS],
    fwk_sequenceTotal
}

static
    fwk_ProjectileData[MAX_PROJECTILE][E_FIREWORK_PROJECTILE_DATA],
    Iterator:fwk_ProjectileIndex<MAX_PROJECTILE>,
    fwk_ExplosionType[MAX_FIREWORK_EXPLOSION_TYPES][E_EXPLOSION_TYPE_DATA],
    FireworkExplosion:fwk_ExplosionTypes,
    fwk_SequenceData[MAX_FIREWORK_SEQUENCES][E_SEQUENCE_DATA],
    FireworkSequence:fwk_Sequences;


stock FireworkExplosion:DefineFireworkExplosionType(models[], totalModels, Float:elevation, Float:distance, spread) {
    if(fwk_ExplosionTypes == MAX_FIREWORK_EXPLOSION_TYPES - FireworkExplosion:1) {
        err("explosion type limit reached");
        return INVALID_EXPLOSION_TYPE;
    }

    for(new i; i < totalModels; i++) {
        fwk_ExplosionType[fwk_ExplosionTypes][fwk_model][i] = models[i];
    }
    fwk_ExplosionType[fwk_ExplosionTypes][fwk_models] = totalModels;
    fwk_ExplosionType[fwk_ExplosionTypes][fwk_elevation] = elevation;
    fwk_ExplosionType[fwk_ExplosionTypes][fwk_distance] = distance;
    fwk_ExplosionType[fwk_ExplosionTypes][fwk_spread] = spread;

    dbg("fireworks", "new explosion type",
        _i("id", _:fwk_ExplosionTypes),
        _i("models", totalModels),
        _f("elevation", elevation),
        _f("distance", distance),
        _i("spread", spread));

    return fwk_ExplosionTypes++;
}

stock FireworkSequence:DefineFireworkExplosionSequence(FireworkExplosion:...) {
    if(fwk_Sequences == MAX_FIREWORK_SEQUENCES - FireworkSequence:1) {
        err("explosion type limit reached");
        return INVALID_SEQUENCE;
    }

    dbg("fireworks", "defining new firework sequence", _i("sequence", _:fwk_Sequences));

    new
        index,
        FireworkExplosion:explID;

    for(new i; i < numargs(); i++) {
        index = fwk_SequenceData[fwk_Sequences][fwk_sequenceTotal];
        explID = FireworkExplosion:getarg(i);

        if(!IsValidFireworkExplosionID(explID)) {
            err("attempted to append invalid explosion id to sequence",
                _i("id", _:explID));
            continue;
        }

        dbg("fireworks", "new explosion in sequence",
            _i("sequence", _:fwk_Sequences),
            _i("index", index),
            _i("expid", _:fwk_SequenceData[fwk_Sequences][fwk_sequenceExp][index]));

        fwk_SequenceData[fwk_Sequences][fwk_sequenceExp][index] = explID;
        fwk_SequenceData[fwk_Sequences][fwk_sequenceTotal]++;
    }

    return fwk_Sequences++;
}

stock CreateFirework(FireworkSequence:sequence, Float:x, Float:y, Float:z) {
    if(!(FireworkSequence:0 <= sequence < fwk_Sequences)) {
        err("attempted to create firework with invalid sequence ID", _i("sequence", _:sequence));
        return;
    }

    _fwk_createAllProjectiles(sequence, 0, x, y, z);

    return;
}

stock bool:IsValidFireworkExplosionID(FireworkExplosion:id) {
    return (FireworkExplosion:0 <= id < fwk_ExplosionTypes);
}

stock bool:IsValidFireworkSequence(FireworkSequence:id) {
    return (FireworkSequence:0 <= id < fwk_Sequences);
}

//
// internal
//

_fwk_createAllProjectiles(FireworkSequence:sequence, index, Float:x, Float:y, Float:z) {
    new FireworkExplosion:explID = fwk_SequenceData[sequence][fwk_sequenceExp][0];
    new Float:angoffset = random((360 / fwk_ExplosionType[explID][fwk_spread]));

    dbg("fireworks", "creating all projectiles for a firework",
        _i("sequence", _:sequence),
        _f("x", x),
        _f("y", y),
        _f("z", z));

    for(new i; i < fwk_ExplosionType[explID][fwk_spread]; i++) {
        _fwk_createProjectile(
            fwk_ExplosionType[explID][fwk_model][random(fwk_ExplosionType[explID][fwk_models])],
            x, y, z, 90.0, 0.0, 0.0,
            i * (360/fwk_ExplosionType[explID][fwk_spread]) + angoffset,
            fwk_ExplosionType[explID][fwk_elevation],
            fwk_ExplosionType[explID][fwk_distance],
            sequence, index);
    }

    return;
}

_fwk_createProjectile(model,
    Float:x, Float:y, Float:z, Float:rx, Float:ry, Float:rz,
    Float:rotation, Float:elevation, Float:distance,
    FireworkSequence:sequence, index
) {
    new id = Iter_Free(fwk_ProjectileIndex);
    if(id == ITER_NONE) {
        err("reached projectile limit");
        return -1;
    }

    new
        Float:to_x = x + ( distance * floatsin(rotation, degrees) * floatcos(elevation + (random(20)-10), degrees) ),
        Float:to_y = y + ( distance * floatcos(rotation, degrees) * floatcos(elevation + (random(20)-10), degrees) ),
        Float:to_z = z + ( distance * floatsin(elevation + (random(20)-10), degrees) );

    fwk_ProjectileData[id][fwk_object] = CreateDynamicObject(model, x, y, z, rx, ry, rz);
    MoveDynamicObject(fwk_ProjectileData[id][fwk_object], to_x, to_y, to_z, 20.0, rx, ry, rz);

    new data[2];
    data[0] = FIREWORK_STREAMER_IDENTIFIER;
    data[1] = id;
    Streamer_SetArrayData(STREAMER_TYPE_OBJECT, fwk_ProjectileData[id][fwk_object], E_STREAMER_EXTRA_ID, data);

    fwk_ProjectileData[id][fwk_sequence] = sequence;
    fwk_ProjectileData[id][fwk_index] = index;

    dbg("fireworks", "created firework projectile",
        _i("fireworkid", id),
        _i("sequence", _:sequence),
        _i("model", model),
        _f("x", x),
        _f("y", y),
        _f("z", z));

    Iter_Add(fwk_ProjectileIndex, id);
    return id;
}

hook OnDynamicObjectMoved(objectid) {
    new data[2];
    Streamer_GetArrayData(STREAMER_TYPE_OBJECT, objectid, E_STREAMER_EXTRA_ID, data);
    if(data[0] != FIREWORK_STREAMER_IDENTIFIER) {
        return Y_HOOKS_CONTINUE_RETURN_0;
    }

    _fwk_onMove(data[1]);

    return Y_HOOKS_CONTINUE_RETURN_0;
}

_fwk_onMove(fireworkid) {
    _fwk_destroyProjectile(fireworkid);

    new
        Float:x,
        Float:y,
        Float:z,
        FireworkSequence:sequence,
        index,
        FireworkExplosion:explID;

    GetDynamicObjectPos(fwk_ProjectileData[fireworkid][fwk_object], x, y, z);

    sequence = fwk_ProjectileData[fireworkid][fwk_sequence];
    index = fwk_ProjectileData[fireworkid][fwk_index];

    if(index + 1 >= fwk_SequenceData[sequence][fwk_sequenceTotal]) {
        dbg("fireworks", "sequence finished",
            _i("sequence", _:sequence),
            _i("index", index),
            _i("fireworkid", fireworkid));
        return;
    }

    explID = fwk_SequenceData[sequence][fwk_sequenceExp][index];
    if(explID == INVALID_EXPLOSION_TYPE) {
        err("sequence contains invalid explosion type",
            _i("sequence", _:sequence),
            _i("index", index));
        return;
    }

    _fwk_createAllProjectiles(sequence, index + 1, x, y, z);

    return;
}

_fwk_destroyProjectile(id) {
    if(!Iter_Contains(fwk_ProjectileIndex, id)) {
        err("attempted to destroy invalid projectile ID");
        return ITER_NONE;
    }

    DestroyDynamicObject(fwk_ProjectileData[id][fwk_object]);
    Iter_SafeRemove(fwk_ProjectileIndex, id, id);

    return id;
}
