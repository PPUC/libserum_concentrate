#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include "sparse-vector.h"
#include "SceneGenerator.h"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include "serum.h"
#include <cstdint>

inline uint16_t ToLittleEndian16(uint16_t value) {
    uint16_t result;
    uint8_t* data = (uint8_t*)&result;
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    return result;
}

inline uint16_t FromLittleEndian16(uint16_t value) {
    const uint8_t* data = (uint8_t*)&value;
    return (uint16_t)data[0] |
           ((uint16_t)data[1] << 8);
}

inline uint32_t ToLittleEndian32(uint32_t value) {
    uint32_t result;
    uint8_t* data = (uint8_t*)&result;
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
    return result;
}

inline uint32_t FromLittleEndian32(uint32_t value) {
    const uint8_t* data = (uint8_t*)&value;
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}


class SerumData
{
public:
    SerumData();
    ~SerumData();

    void Clear();
    bool SaveToFile(const char *filename);
    bool LoadFromFile(const char *filename, const uint8_t flags);

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
    bool is256x64;

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

    SceneGenerator *sceneGenerator;

private:
    uint8_t m_loadFlags = 0;

    friend class cereal::access;

    template <class Archive>
    void serialize(Archive &ar)
    {
        ar(rname, SerumVersion,
           fwidth, fheight, fwidthx, fheightx,
           nframes, nocolors, nccolors,
           ncompmasks, nmovmasks, nsprites,
           nbackgrounds, is256x64,
           hashcodes, shapecompmode, compmaskID,
           movrctID, compmasks, movrcts,
           cpal, isextraframe, cframes,
           cframesn, cframesnx, dynamasks,
           dynamasksx, dyna4cols, dyna4colsn,
           dyna4colsnx, framesprites,
           spritedescriptionso, spritedescriptionsc,
           isextrasprite, spriteoriginal,
           spritemaskx, spritecolored,
           spritecoloredx, activeframes,
           colorrotations, colorrotationsn,
           colorrotationsnx, spritedetdwords,
           spritedetdwordpos, spritedetareas,
           triggerIDs, framespriteBB,
           isextrabackground, backgroundframes,
           backgroundframesn, backgroundframesnx,
           backgroundIDs, backgroundBB,
           backgroundmask, backgroundmaskx,
           dynashadowsdiro, dynashadowscolo,
           dynashadowsdirx, dynashadowscolx,
           dynasprite4cols, dynasprite4colsx,
           dynaspritemasks, dynaspritemasksx,
           sprshapemode);

        if constexpr (Archive::is_saving::value)
        {
            ar(sceneGenerator ? sceneGenerator->getSceneData() : std::vector<SceneData>{});
        }
        else
        {
            if (SERUM_V2 == SerumVersion && ((fheight == 32 && !(m_loadFlags & FLAG_REQUEST_64P_FRAMES)) || (fheight == 64 && !(m_loadFlags & FLAG_REQUEST_32P_FRAMES))))
            {
                isextraframe.clearIndex();
                isextrabackground.clearIndex();
                isextrasprite.clearIndex();
            }

            cframesnx.setParent(&isextraframe);
            dynamasksx.setParent(&isextraframe);
            dyna4colsnx.setParent(&isextraframe);
            spritemaskx.setParent(&isextrasprite);
            spritecoloredx.setParent(&isextrasprite);
            colorrotationsnx.setParent(&isextraframe);
            framespriteBB.setParent(&framesprites);
            backgroundframesnx.setParent(&isextrabackground);
            backgroundmask.setParent(&backgroundIDs);
            backgroundmaskx.setParent(&backgroundIDs);
            dynashadowsdirx.setParent(&isextraframe);
            dynashadowscolx.setParent(&isextraframe);
            dynasprite4colsx.setParent(&isextraframe);
            dynaspritemasksx.setParent(&isextraframe);
            backgroundBB.setParent(&backgroundIDs);

            std::vector<SceneData> loadedScenes;
            ar(loadedScenes);
            if (sceneGenerator)
            {
                sceneGenerator->setSceneData(std::move(loadedScenes));
                sceneGenerator->setDepth(nocolors == 16 ? 4 : 2);
            }
        }
    }
};
