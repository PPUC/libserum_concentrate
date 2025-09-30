#pragma once

#include <cstdint>
#include "sparse-vector.h"
#include "SceneGenerator.h"
//#include <cereal/access.hpp>

class SerumData {
public:
    SerumData();
    ~SerumData();

    void Clear();

    // Make private members accessible to cereal
    //friend class cereal::access;

    // Header data
    char rname[64];
    uint8_t SerumVersion;
    uint16_t concentrateFileVersion;
    uint32_t fwidth, fheight;
    uint32_t fwidthx, fheightx;
    uint32_t nframes;
    uint32_t nocolors, nccolors;
    uint32_t ncompmasks, nmovmasks;
    uint32_t nsprites;
    uint16_t nbackgrounds;

    // Vector data
    SparseVector<uint32_t> hashcodes;
    SparseVector<uint8_t> shapecompmode;
    SparseVector<uint8_t> compmaskID;
    SparseVector<uint8_t> movrctID;
    SparseVector<uint8_t> compmasks;
    SparseVector<uint8_t> movrcts; // Currently unused
    SparseVector<uint8_t> cpal;
    SparseVector<uint8_t> isextraframe;
    SparseVector<uint8_t> cframes;
    SparseVector<uint16_t> cframesn;
    SparseVector<uint16_t> cframesnx;
    SparseVector<uint8_t> dynamasks;
    SparseVector<uint8_t> dynamasksx;
    SparseVector<uint8_t> dyna4cols;
    SparseVector<uint16_t> dyna4colsn;
    SparseVector<uint16_t> dyna4colsnx;
    SparseVector<uint8_t> framesprites;
    SparseVector<uint8_t> spritedescriptionso;
    SparseVector<uint8_t> spritedescriptionsc;
    SparseVector<uint8_t> isextrasprite;
    SparseVector<uint8_t> spriteoriginal;
    SparseVector<uint8_t> spritemaskx;
    SparseVector<uint16_t> spritecolored;
    SparseVector<uint16_t> spritecoloredx;
    SparseVector<uint8_t> activeframes;
    SparseVector<uint8_t> colorrotations;
    SparseVector<uint16_t> colorrotationsn;
    SparseVector<uint16_t> colorrotationsnx;
    SparseVector<uint32_t> spritedetdwords;
    SparseVector<uint16_t> spritedetdwordpos;
    SparseVector<uint16_t> spritedetareas;
    SparseVector<uint32_t> triggerIDs;
    SparseVector<uint16_t> framespriteBB;
    SparseVector<uint8_t> isextrabackground;
    SparseVector<uint8_t> backgroundframes;
    SparseVector<uint16_t> backgroundframesn;
    SparseVector<uint16_t> backgroundframesnx;
    SparseVector<uint16_t> backgroundIDs;
    SparseVector<uint16_t> backgroundBB;
    SparseVector<uint8_t> backgroundmask;
    SparseVector<uint8_t> backgroundmaskx;
    SparseVector<uint8_t> dynashadowsdiro;
    SparseVector<uint16_t> dynashadowscolo;
    SparseVector<uint8_t> dynashadowsdirx;
    SparseVector<uint16_t> dynashadowscolx;
    SparseVector<uint16_t> dynasprite4cols;
    SparseVector<uint16_t> dynasprite4colsx;
    SparseVector<uint8_t> dynaspritemasks;
    SparseVector<uint8_t> dynaspritemasksx;
    SparseVector<uint8_t> sprshapemode;

//private:
//    template<class Archive>
//    void serialize(Archive & ar) {
//        // Add serialization code here
//    }
};
