#include "SerumData.h"
#include "serum-version.h"
#include <cstring>

SerumData::SerumData() : SerumVersion(0),
                         concentrateFileVersion(SERUM_CONCENTRATE_VERSION),
                         is256x64(false),
                         hashcodes(0, true),
                         shapecompmode(0),
                         compmaskID(255),
                         movrctID(0),
                         compmasks(0),
                         movrcts(0),
                         cpal(0),
                         isextraframe(0, true),
                         cframes(0, false, true),
                         cframesn(0, false, true),
                         cframesnx(0, false, true),
                         dynamasks(255, false, true),
                         dynamasksx(255, false, true),
                         dyna4cols(0),
                         dyna4colsn(0),
                         dyna4colsnx(0),
                         framesprites(255),
                         spritedescriptionso(0),
                         spritedescriptionsc(0),
                         isextrasprite(0, true),
                         spriteoriginal(255), // Do not compress because GetSpriteSize seems to have an issue with it.
                         spritemaskx(255),    // Do not compress because GetSpriteSize seems to have an issue with it.
                         spritecolored(0, false, true),
                         spritecoloredx(0, false, true),
                         activeframes(1),
                         colorrotations(0),
                         colorrotationsn(0),
                         colorrotationsnx(0),
                         spritedetdwords(0),
                         spritedetdwordpos(0),
                         spritedetareas(0),
                         triggerIDs(0xffffffff),
                         framespriteBB(0, false, true),
                         isextrabackground(0, true),
                         backgroundframes(0, false, true),
                         backgroundframesn(0, false, true),
                         backgroundframesnx(0, false, true),
                         backgroundIDs(0xffff),
                         backgroundBB(0),
                         backgroundmask(0, false, true),
                         backgroundmaskx(0, false, true),
                         dynashadowsdiro(0),
                         dynashadowscolo(0),
                         dynashadowsdirx(0),
                         dynashadowscolx(0),
                         dynasprite4cols(0),
                         dynasprite4colsx(0),
                         dynaspritemasks(255, false, true),
                         dynaspritemasksx(255, false, true),
                         sprshapemode(0)
{
    sceneGenerator = new SceneGenerator();
}

SerumData::~SerumData()
{
}

void SerumData::Clear()
{
    hashcodes.clear();
    shapecompmode.clear();
    compmaskID.clear();
    compmasks.clear();
    cpal.clear();
    isextraframe.clear();
    cframesn.clear();
    cframesnx.clear();
    cframes.clear();
    dynamasks.clear();
    dynamasksx.clear();
    dyna4cols.clear();
    dyna4colsn.clear();
    dyna4colsnx.clear();
    framesprites.clear();
    spritedescriptionso.clear();
    spritedescriptionsc.clear();
    isextrasprite.clear();
    spriteoriginal.clear();
    spritemaskx.clear();
    spritecolored.clear();
    spritecoloredx.clear();
    activeframes.clear();
    colorrotations.clear();
    colorrotationsn.clear();
    colorrotationsnx.clear();
    spritedetareas.clear();
    spritedetdwords.clear();
    spritedetdwordpos.clear();
    triggerIDs.clear();
    framespriteBB.clear();
    isextrabackground.clear();
    backgroundframes.clear();
    backgroundframesn.clear();
    backgroundframesnx.clear();
    backgroundIDs.clear();
    backgroundBB.clear();
    backgroundmask.clear();
    backgroundmaskx.clear();
    dynashadowsdiro.clear();
    dynashadowscolo.clear();
    dynashadowsdirx.clear();
    dynashadowscolx.clear();
    dynasprite4cols.clear();
    dynasprite4colsx.clear();
    dynaspritemasks.clear();
    dynaspritemasksx.clear();
    sprshapemode.clear();
}

bool SerumData::SaveToFile(const char *filename)
{
    try
    {
        Log("Writing %s", filename);
        // Serialize to memory buffer first
        std::ostringstream ss(std::ios::binary);
        {
            cereal::PortableBinaryOutputArchive archive(ss);
            archive(*this);
        }
        std::string data = ss.str();

        // Write data directly to file (no compression)
        FILE *fp = fopen(filename, "wb");
        if (!fp)
        {
            Log("Failed to open %s for writing", filename);
            return false;
        }

        // Write magic string first
        const char magic[] = "CROM";
        fwrite(magic, 1, 4, fp);

        // Write version
        uint16_t littleVersion = ToLittleEndian16(concentrateFileVersion);
        fwrite(&littleVersion, sizeof(uint16_t), 1, fp);

        // Write data size
        uint32_t dataSize = (uint32_t)data.size();
        uint32_t littleEndianSize = ToLittleEndian32(dataSize);
        fwrite(&littleEndianSize, sizeof(uint32_t), 1, fp);

        // Write serialized data directly (no compression)
        fwrite(data.data(), 1, dataSize, fp);
        fclose(fp);

        Log("Writing %s finished", filename);
        return true;
    }
    catch (const std::exception &e)
    {
        Log("Exception when writing %s: %s", filename, e.what());
        return false;
    }
    catch (...)
    {
        Log("Failed to write %s", filename);
        return false;
    }
}

bool SerumData::LoadFromFile(const char *filename, const uint8_t flags)
{
    m_loadFlags = flags;

    try
    {
        FILE *fp = fopen(filename, "rb");
        if (!fp)
        {
            Log("Failed to open %s", filename);
            return false;
        }

        // Read and verify magic string
        char magic[5] = {0};
        if (fread(magic, 1, 4, fp) != 4 || strcmp(magic, "CROM") != 0)
        {
            Log("Wrong header in %s", filename);
            fclose(fp);
            return false;
        }

        uint16_t littleEndianVersion;
        if (fread(&littleEndianVersion, sizeof(uint16_t), 1, fp) != 1)
        {
            Log("Failed to detect cROMc version of %s", filename);
            fclose(fp);
            return false;
        }
        concentrateFileVersion = FromLittleEndian16(littleEndianVersion);
        Log("cROMc version %d", concentrateFileVersion);

        // Read data size
        uint32_t littleEndianSize;
        if (fread(&littleEndianSize, sizeof(uint32_t), 1, fp) != 1)
        {
            Log("Failed to detect cROMc size of %s", filename);
            fclose(fp);
            return false;
        }
        uint32_t dataSize = FromLittleEndian32(littleEndianSize);
        Log("cROMc size %u", dataSize);

        // Validate size
        if (dataSize == 0)
        {
            Log("Invalid file size detected in %s", filename);
            fclose(fp);
            return false;
        }

        // Read serialized data directly (no decompression needed)
        std::vector<char> fileData(dataSize);
        if (fread(fileData.data(), 1, dataSize, fp) != dataSize)
        {
            Log("Failed to read data from %s", filename);
            fclose(fp);
            return false;
        }
        fclose(fp);

        // Deserialize from memory buffer directly
        Log("Loading %s", filename);
        std::istringstream ss(std::string(fileData.data(), dataSize), std::ios::binary);
        {
            cereal::PortableBinaryInputArchive archive(ss);
            archive(*this);
        }

        return true;
    }
    catch (const std::exception &e)
    {
        Log("Exception when opening %s: %s", filename, e.what());
        return false;
    }
    catch (...)
    {
        Log("Unknown exception when opening %s", filename);
        return false;
    }
}

void SerumData::Log(const char *format, ...)
{
    if (!m_logCallback)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    (*(m_logCallback))(format, args, m_logUserData);
    va_end(args);
}
