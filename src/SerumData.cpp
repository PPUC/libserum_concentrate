#include "SerumData.h"

SerumData::SerumData() :
    SerumVersion(0),
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
    spriteoriginal(0),
    spritemaskx(255),
    spritecolored(0),
    spritecoloredx(0),
    activeframes(1),
    colorrotations(0),
    colorrotationsn(0),
    colorrotationsnx(0),
    spritedetdwords(0),
    spritedetdwordpos(0),
    spritedetareas(0),
    triggerIDs(0xffffffff),
    framespriteBB(0),
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
    dynaspritemasks(255),
    dynaspritemasksx(255),
    sprshapemode(0)
{
}

SerumData::~SerumData() {
}

void SerumData::Clear() {
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
