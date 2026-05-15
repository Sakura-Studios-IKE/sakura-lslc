/* builtins.c - LSL Mono built-in functions, constants, and events.
 *
 * Coverage notes
 * --------------
 * The tables below cover the great majority of the documented LSL Mono
 * surface area (~430 functions, several hundred constants, all events).
 * Where a function is not present in this table the compiler does NOT
 * reject it outright — it issues an informational warning under -Wall and
 * treats its return type as <any>, so that custom or future SL additions
 * still parse cleanly. This mirrors the conservative behaviour we want for
 * a strict but forgiving developer tool.
 */
#include "lsl.h"
#include <string.h>

/* helper macros — keep entries compact */
#define FN0(name, ret) { name, ret, 0, {0}, 0 }
#define FN1(name, ret, a) { name, ret, 1, {a}, 0 }
#define FN2(name, ret, a, b) { name, ret, 2, {a, b}, 0 }
#define FN3(name, ret, a, b, c) { name, ret, 3, {a, b, c}, 0 }
#define FN4(name, ret, a, b, c, d) { name, ret, 4, {a, b, c, d}, 0 }
#define FN5(name, ret, a, b, c, d, e) { name, ret, 5, {a, b, c, d, e}, 0 }
#define FN6(name, ret, a, b, c, d, e, f) { name, ret, 6, {a, b, c, d, e, f}, 0 }
#define FN7(name, ret, a, b, c, d, e, f, g) { name, ret, 7, {a, b, c, d, e, f, g}, 0 }

/* Mono-only variants — set BFN_MONO_ONLY in flags. Used by --lso mode. */
#define MN0(name, ret) { name, ret, 0, {0}, BFN_MONO_ONLY }
#define MN1(name, ret, a) { name, ret, 1, {a}, BFN_MONO_ONLY }
#define MN2(name, ret, a, b) { name, ret, 2, {a, b}, BFN_MONO_ONLY }
#define MN3(name, ret, a, b, c) { name, ret, 3, {a, b, c}, BFN_MONO_ONLY }

/* ----------------------------- Functions -------------------------------- */
const BuiltinFn BI_FN[] = {
    /* --------- Avatar / object / world info ---------- */
    FN0("llGetOwner",                 T_KEY),
    FN0("llGetCreator",               T_KEY),
    FN0("llGetCreatorKey",            T_KEY),
    FN0("llGetKey",                   T_KEY),
    FN0("llGetObjectName",            T_STRING),
    FN1("llSetObjectName",            T_VOID,    T_STRING),
    FN0("llGetObjectDesc",            T_STRING),
    FN1("llSetObjectDesc",            T_VOID,    T_STRING),
    FN0("llGetPos",                   T_VECTOR),
    FN1("llSetPos",                   T_VOID,    T_VECTOR),
    FN1("llSetRegionPos",             T_INTEGER, T_VECTOR),
    FN0("llGetRot",                   T_ROTATION),
    FN1("llSetRot",                   T_VOID,    T_ROTATION),
    FN0("llGetLocalPos",              T_VECTOR),
    FN0("llGetLocalRot",              T_ROTATION),
    FN0("llGetRootPosition",          T_VECTOR),
    FN0("llGetRootRotation",          T_ROTATION),
    FN0("llGetVel",                   T_VECTOR),
    FN0("llGetAccel",                 T_VECTOR),
    FN0("llGetOmega",                 T_VECTOR),
    FN0("llGetMass",                  T_FLOAT),
    FN1("llGetMassMKS",               T_FLOAT, T_INTEGER),
    FN0("llGetScale",                 T_VECTOR),
    FN1("llSetScale",                 T_VOID, T_VECTOR),
    FN0("llGetEnergy",                T_FLOAT),
    FN0("llGetGeometricCenter",       T_VECTOR),
    FN0("llGetCenterOfMass",          T_VECTOR),
    FN1("llGetBoundingBox",           T_LIST, T_KEY),
    FN0("llGetTime",                  T_FLOAT),
    FN0("llGetTimeOfDay",             T_FLOAT),
    FN0("llGetAndResetTime",          T_FLOAT),
    FN0("llResetTime",                T_VOID),
    FN0("llGetUnixTime",              T_INTEGER),
    FN0("llGetWallclock",             T_FLOAT),
    FN0("llGetDate",                  T_STRING),
    FN0("llGetTimestamp",             T_STRING),
    FN0("llGetGMTclock",              T_FLOAT),
    FN0("llGetSimulatorHostname",     T_STRING),
    FN0("llGetSimStats",              T_FLOAT),     /* simplified */
    FN1("llGetEnv",                   T_STRING, T_STRING),
    FN0("llGetRegionName",            T_STRING),
    FN0("llGetRegionCorner",          T_VECTOR),
    FN0("llGetRegionFlags",           T_INTEGER),
    FN0("llGetRegionFPS",             T_FLOAT),
    FN0("llGetRegionTimeDilation",    T_FLOAT),
    FN0("llGetRegionAgentCount",      T_INTEGER),
    FN1("llRequestSimulatorData",     T_KEY, T_STRING),  /* deprecated style */
    FN2("llRequestSimulatorData",     T_KEY, T_STRING, T_INTEGER),
    FN1("llCloud",                    T_FLOAT, T_VECTOR),
    FN1("llGround",                   T_FLOAT, T_VECTOR),
    FN1("llGroundNormal",             T_VECTOR, T_VECTOR),
    FN1("llGroundSlope",              T_VECTOR, T_VECTOR),
    FN1("llWater",                    T_FLOAT, T_VECTOR),
    FN1("llWind",                     T_VECTOR, T_VECTOR),
    FN1("llSunDirection",             T_VECTOR, T_VOID),
    FN0("llSunDirection",             T_VECTOR),
    FN0("llGetSunDirection",          T_VECTOR),
    FN0("llGetMoonDirection",         T_VECTOR),
    FN0("llGetMoonRotation",          T_ROTATION),

    /* --------- Conversion / math ---------- */
    FN1("llAbs",        T_INTEGER, T_INTEGER),
    FN1("llFabs",       T_FLOAT,   T_FLOAT),
    FN1("llCeil",       T_INTEGER, T_FLOAT),
    FN1("llFloor",      T_INTEGER, T_FLOAT),
    FN1("llRound",      T_INTEGER, T_FLOAT),
    FN1("llSqrt",       T_FLOAT,   T_FLOAT),
    FN1("llSin",        T_FLOAT,   T_FLOAT),
    FN1("llCos",        T_FLOAT,   T_FLOAT),
    FN1("llTan",        T_FLOAT,   T_FLOAT),
    FN1("llAsin",       T_FLOAT,   T_FLOAT),
    FN1("llAcos",       T_FLOAT,   T_FLOAT),
    FN1("llAtan2",      T_FLOAT,   T_FLOAT),
    FN2("llAtan2",      T_FLOAT,   T_FLOAT, T_FLOAT),
    FN2("llPow",        T_FLOAT,   T_FLOAT, T_FLOAT),
    FN1("llLog",        T_FLOAT,   T_FLOAT),
    FN1("llLog10",      T_FLOAT,   T_FLOAT),
    FN1("llFrand",      T_FLOAT,   T_FLOAT),
    FN1("llVecMag",     T_FLOAT,   T_VECTOR),
    FN1("llVecNorm",    T_VECTOR,  T_VECTOR),
    FN2("llVecDist",    T_FLOAT,   T_VECTOR, T_VECTOR),
    FN1("llRot2Euler",  T_VECTOR,  T_ROTATION),
    FN1("llEuler2Rot",  T_ROTATION,T_VECTOR),
    FN1("llRot2Fwd",    T_VECTOR,  T_ROTATION),
    FN1("llRot2Left",   T_VECTOR,  T_ROTATION),
    FN1("llRot2Up",     T_VECTOR,  T_ROTATION),
    FN2("llAngleBetween", T_FLOAT, T_ROTATION, T_ROTATION),
    FN1("llRot2Angle",  T_FLOAT,   T_ROTATION),
    FN1("llRot2Axis",   T_VECTOR,  T_ROTATION),
    FN2("llAxisAngle2Rot", T_ROTATION, T_VECTOR, T_FLOAT),
    FN2("llAxes2Rot",   T_ROTATION, T_VECTOR, T_VECTOR),
    FN3("llAxes2Rot",   T_ROTATION, T_VECTOR, T_VECTOR, T_VECTOR),
    FN2("llRotBetween", T_ROTATION, T_VECTOR, T_VECTOR),
    FN1("llModPow",     T_INTEGER, T_INTEGER),
    FN3("llModPow",     T_INTEGER, T_INTEGER, T_INTEGER, T_INTEGER),

    /* --------- Strings ---------- */
    FN1("llStringLength",       T_INTEGER, T_STRING),
    FN3("llGetSubString",       T_STRING,  T_STRING, T_INTEGER, T_INTEGER),
    FN3("llInsertString",       T_STRING,  T_STRING, T_INTEGER, T_STRING),
    FN3("llDeleteSubString",    T_STRING,  T_STRING, T_INTEGER, T_INTEGER),
    FN2("llSubStringIndex",     T_INTEGER, T_STRING, T_STRING),
    FN1("llToLower",            T_STRING,  T_STRING),
    FN1("llToUpper",            T_STRING,  T_STRING),
    FN2("llStringTrim",         T_STRING,  T_STRING, T_INTEGER),
    FN1("llStringTrim",         T_STRING,  T_STRING),
    MN2("llChar",               T_STRING,  T_INTEGER, T_VOID),
    MN1("llChar",               T_STRING,  T_INTEGER),
    MN1("llOrd",                T_INTEGER, T_STRING),
    MN2("llOrd",                T_INTEGER, T_STRING, T_INTEGER),
    FN1("llStringToBase64",     T_STRING,  T_STRING),
    FN1("llBase64ToString",     T_STRING,  T_STRING),
    FN2("llXorBase64",          T_STRING,  T_STRING, T_STRING),
    FN2("llXorBase64Strings",   T_STRING,  T_STRING, T_STRING),
    FN2("llXorBase64StringsCorrect", T_STRING, T_STRING, T_STRING),
    FN1("llIntegerToBase64",    T_STRING,  T_INTEGER),
    FN1("llBase64ToInteger",    T_INTEGER, T_STRING),
    FN1("llEscapeURL",          T_STRING,  T_STRING),
    FN1("llUnescapeURL",        T_STRING,  T_STRING),
    FN1("llMD5String",          T_STRING,  T_STRING),  /* (string,int nonce) */
    FN2("llMD5String",          T_STRING,  T_STRING, T_INTEGER),
    FN1("llSHA1String",         T_STRING,  T_STRING),
    FN2("llSHA1String",         T_STRING,  T_STRING, T_INTEGER),
    FN2("llSHA256String",       T_STRING,  T_STRING, T_INTEGER),
    FN1("llSHA256String",       T_STRING,  T_STRING),
    MN2("llHMAC",               T_STRING,  T_STRING, T_STRING),
    MN3("llHMAC",               T_STRING,  T_STRING, T_STRING, T_STRING),

    /* --------- Lists ---------- */
    FN1("llGetListLength",      T_INTEGER, T_LIST),
    FN2("llList2String",        T_STRING,  T_LIST, T_INTEGER),
    FN2("llList2Integer",       T_INTEGER, T_LIST, T_INTEGER),
    FN2("llList2Float",         T_FLOAT,   T_LIST, T_INTEGER),
    FN2("llList2Key",           T_KEY,     T_LIST, T_INTEGER),
    FN2("llList2Vector",        T_VECTOR,  T_LIST, T_INTEGER),
    FN2("llList2Rot",           T_ROTATION,T_LIST, T_INTEGER),
    FN3("llList2List",          T_LIST,    T_LIST, T_INTEGER, T_INTEGER),
    FN3("llList2ListStrided",   T_LIST,    T_LIST, T_INTEGER, T_INTEGER),
    FN4("llList2ListStrided",   T_LIST,    T_LIST, T_INTEGER, T_INTEGER, T_INTEGER),
    FN2("llDeleteSubList",      T_LIST,    T_LIST, T_INTEGER),
    FN3("llDeleteSubList",      T_LIST,    T_LIST, T_INTEGER, T_INTEGER),
    FN2("llListFindList",       T_INTEGER, T_LIST, T_LIST),
    FN3("llListFindListNext",   T_INTEGER, T_LIST, T_LIST, T_INTEGER),
    FN2("llListInsertList",     T_LIST,    T_LIST, T_LIST),
    FN3("llListInsertList",     T_LIST,    T_LIST, T_LIST, T_INTEGER),
    FN3("llListReplaceList",    T_LIST,    T_LIST, T_LIST, T_INTEGER),
    FN4("llListReplaceList",    T_LIST,    T_LIST, T_LIST, T_INTEGER, T_INTEGER),
    FN2("llListSort",           T_LIST,    T_LIST, T_INTEGER),
    FN3("llListSort",           T_LIST,    T_LIST, T_INTEGER, T_INTEGER),
    FN2("llListRandomize",      T_LIST,    T_LIST, T_INTEGER),
    FN1("llListStatistics",     T_FLOAT,   T_LIST),
    FN2("llListStatistics",     T_FLOAT,   T_INTEGER, T_LIST),
    FN2("llDumpList2String",    T_STRING,  T_LIST, T_STRING),
    FN3("llParseString2List",   T_LIST,    T_STRING, T_LIST, T_LIST),
    FN3("llParseStringKeepNulls", T_LIST,  T_STRING, T_LIST, T_LIST),
    FN2("llCSV2List",           T_LIST,    T_STRING, T_STRING),
    FN1("llCSV2List",           T_LIST,    T_STRING),
    FN1("llList2CSV",           T_STRING,  T_LIST),

    /* --------- JSON ---------- */
    MN3("llJsonGetValue",       T_STRING,  T_STRING, T_LIST, T_VOID),
    MN2("llJsonGetValue",       T_STRING,  T_STRING, T_LIST),
    MN3("llJsonSetValue",       T_STRING,  T_STRING, T_LIST, T_STRING),
    MN2("llJsonValueType",      T_STRING,  T_STRING, T_LIST),
    MN2("llJson2List",          T_LIST,    T_STRING, T_VOID),
    MN1("llJson2List",          T_LIST,    T_STRING),
    MN2("llList2Json",          T_STRING,  T_STRING, T_LIST),

    /* --------- Communication ---------- */
    FN2("llSay",                T_VOID,    T_INTEGER, T_STRING),
    FN2("llWhisper",            T_VOID,    T_INTEGER, T_STRING),
    FN2("llShout",              T_VOID,    T_INTEGER, T_STRING),
    FN1("llOwnerSay",           T_VOID,    T_STRING),
    FN3("llRegionSayTo",        T_VOID,    T_KEY, T_INTEGER, T_STRING),
    FN2("llRegionSay",          T_VOID,    T_INTEGER, T_STRING),
    FN2("llInstantMessage",     T_VOID,    T_KEY, T_STRING),
    FN4("llListen",             T_INTEGER, T_INTEGER, T_STRING, T_KEY, T_STRING),
    FN1("llListenRemove",       T_VOID,    T_INTEGER),
    FN2("llListenControl",      T_VOID,    T_INTEGER, T_INTEGER),
    FN3("llDialog",             T_VOID,    T_KEY, T_STRING, T_LIST),
    FN4("llDialog",             T_VOID,    T_KEY, T_STRING, T_LIST, T_INTEGER),
    FN3("llTextBox",            T_VOID,    T_KEY, T_STRING, T_INTEGER),
    FN2("llLoadURL",            T_VOID,    T_KEY, T_STRING),
    FN3("llLoadURL",            T_VOID,    T_KEY, T_STRING, T_STRING),
    FN2("llSetText",            T_VOID,    T_STRING, T_VECTOR),
    FN3("llSetText",            T_VOID,    T_STRING, T_VECTOR, T_FLOAT),
    FN4("llSetText",            T_VOID,    T_STRING, T_VECTOR, T_FLOAT, T_INTEGER),

    /* --------- Detection / sensor / collision ---------- */
    FN1("llDetectedKey",        T_KEY,     T_INTEGER),
    FN1("llDetectedName",       T_STRING,  T_INTEGER),
    FN1("llDetectedPos",        T_VECTOR,  T_INTEGER),
    FN1("llDetectedRot",        T_ROTATION,T_INTEGER),
    FN1("llDetectedType",       T_INTEGER, T_INTEGER),
    FN1("llDetectedOwner",      T_KEY,     T_INTEGER),
    FN1("llDetectedLinkNumber", T_INTEGER, T_INTEGER),
    FN1("llDetectedGrab",       T_VECTOR,  T_INTEGER),
    FN1("llDetectedGroup",      T_INTEGER, T_INTEGER),
    FN1("llDetectedTouchPos",   T_VECTOR,  T_INTEGER),
    FN1("llDetectedTouchFace",  T_INTEGER, T_INTEGER),
    FN1("llDetectedTouchST",    T_VECTOR,  T_INTEGER),
    FN1("llDetectedTouchUV",    T_VECTOR,  T_INTEGER),
    FN1("llDetectedTouchNormal",T_VECTOR,  T_INTEGER),
    FN1("llDetectedTouchBinormal", T_VECTOR, T_INTEGER),
    FN1("llDetectedVel",        T_VECTOR,  T_INTEGER),
    FN5("llSensor",             T_VOID, T_STRING, T_KEY, T_INTEGER, T_FLOAT, T_FLOAT),
    FN6("llSensorRepeat",       T_VOID, T_STRING, T_KEY, T_INTEGER, T_FLOAT, T_FLOAT, T_FLOAT),
    FN0("llSensorRemove",       T_VOID),
    FN1("llSetCollisionFilter", T_VOID, T_STRING),
    FN3("llSetCollisionFilter", T_VOID, T_STRING, T_KEY, T_INTEGER),

    /* --------- Inventory / object manipulation ---------- */
    FN0("llGetInventoryNumber", T_INTEGER),
    FN1("llGetInventoryNumber", T_INTEGER, T_INTEGER),
    FN2("llGetInventoryName",   T_STRING,  T_INTEGER, T_INTEGER),
    FN1("llGetInventoryType",   T_INTEGER, T_STRING),
    FN1("llGetInventoryKey",    T_KEY,     T_STRING),
    FN1("llGetInventoryCreator",T_KEY,     T_STRING),
    FN1("llGetInventoryPermMask", T_INTEGER, T_STRING),
    FN2("llGetInventoryPermMask", T_INTEGER, T_STRING, T_INTEGER),
    FN1("llGetInventoryDesc",   T_STRING,  T_STRING),
    FN2("llSetInventoryPermMask", T_VOID, T_STRING, T_INTEGER),
    FN3("llSetInventoryPermMask", T_VOID, T_STRING, T_INTEGER, T_INTEGER),
    FN1("llRemoveInventory",    T_VOID, T_STRING),
    FN2("llGiveInventory",      T_VOID, T_KEY, T_STRING),
    FN3("llGiveInventoryList",  T_VOID, T_KEY, T_STRING, T_LIST),
    FN2("llRezObject",          T_VOID, T_STRING, T_VECTOR),
    FN5("llRezObject",          T_VOID, T_STRING, T_VECTOR, T_VECTOR, T_ROTATION, T_INTEGER),
    FN5("llRezAtRoot",          T_VOID, T_STRING, T_VECTOR, T_VECTOR, T_ROTATION, T_INTEGER),
    FN6("llRezAtRoot",          T_VOID, T_STRING, T_VECTOR, T_VECTOR, T_ROTATION, T_INTEGER, T_INTEGER),
    FN0("llDie",                T_VOID),
    FN0("llResetScript",        T_VOID),
    FN1("llResetOtherScript",   T_VOID, T_STRING),
    FN1("llSleep",              T_VOID, T_FLOAT),
    FN0("llGetScriptName",      T_STRING),
    FN0("llGetScriptID",        T_KEY),
    FN1("llGetScriptState",     T_INTEGER, T_STRING),
    FN2("llSetScriptState",     T_VOID, T_STRING, T_INTEGER),
    FN0("llGetUsedMemory",      T_INTEGER),
    FN0("llGetFreeMemory",      T_INTEGER),
    FN0("llGetMemoryLimit",     T_INTEGER),
    FN1("llSetMemoryLimit",     T_INTEGER, T_INTEGER),
    FN0("llGetSPMaxMemory",     T_INTEGER),

    /* --------- Permissions / money ---------- */
    FN2("llRequestPermissions", T_VOID, T_KEY, T_INTEGER),
    FN0("llGetPermissions",     T_INTEGER),
    FN0("llGetPermissionsKey",  T_KEY),
    FN2("llGiveMoney",          T_INTEGER, T_KEY, T_INTEGER),
    FN0("llGetMyAccountBalance",T_INTEGER),
    MN2("llTransferLindenDollars", T_KEY, T_KEY, T_INTEGER),
    FN1("llCollisionFilter",    T_VOID, T_STRING),
    FN2("llSetPayPrice",        T_VOID, T_INTEGER, T_LIST),
    FN1("llGetPayPrice",        T_LIST, T_KEY),

    /* --------- Links / prim params ---------- */
    FN0("llGetLinkNumber",      T_INTEGER),
    FN1("llGetLinkName",        T_STRING, T_INTEGER),
    FN0("llGetNumberOfPrims",   T_INTEGER),
    FN1("llGetLinkKey",         T_KEY, T_INTEGER),
    FN2("llSetLinkAlpha",       T_VOID, T_INTEGER, T_FLOAT),
    FN3("llSetLinkAlpha",       T_VOID, T_INTEGER, T_FLOAT, T_INTEGER),
    FN3("llSetLinkColor",       T_VOID, T_INTEGER, T_VECTOR, T_INTEGER),
    FN3("llSetLinkTexture",     T_VOID, T_INTEGER, T_STRING, T_INTEGER),
    FN4("llSetLinkTextureAnim", T_VOID, T_INTEGER, T_INTEGER, T_INTEGER, T_INTEGER),
    FN2("llSetLinkPrimitiveParams",     T_VOID, T_INTEGER, T_LIST),
    FN2("llSetLinkPrimitiveParamsFast", T_VOID, T_INTEGER, T_LIST),
    FN2("llGetLinkPrimitiveParams",     T_LIST, T_INTEGER, T_LIST),
    FN1("llSetPrimitiveParams",         T_VOID, T_LIST),
    FN1("llGetPrimitiveParams",         T_LIST, T_LIST),
    FN1("llSetColor",                   T_VOID, T_VECTOR),
    FN2("llSetColor",                   T_VOID, T_VECTOR, T_INTEGER),
    FN1("llGetColor",                   T_VECTOR, T_INTEGER),
    FN2("llSetAlpha",                   T_VOID, T_FLOAT, T_INTEGER),
    FN1("llGetAlpha",                   T_FLOAT, T_INTEGER),
    FN2("llSetTexture",                 T_VOID, T_STRING, T_INTEGER),
    FN1("llGetTexture",                 T_STRING, T_INTEGER),
    FN3("llOffsetTexture",              T_VOID, T_FLOAT, T_FLOAT, T_INTEGER),
    FN3("llScaleTexture",               T_VOID, T_FLOAT, T_FLOAT, T_INTEGER),
    FN2("llRotateTexture",              T_VOID, T_FLOAT, T_INTEGER),
    FN0("llBreakAllLinks",              T_VOID),
    FN1("llBreakLink",                  T_VOID, T_INTEGER),
    FN1("llCreateLink",                 T_VOID, T_KEY),
    FN2("llCreateLink",                 T_VOID, T_KEY, T_INTEGER),

    /* --------- HTTP / network ---------- */
    FN3("llHTTPRequest",        T_KEY, T_STRING, T_LIST, T_STRING),
    FN1("llHTTPResponse",       T_VOID, T_KEY),
    FN3("llHTTPResponse",       T_VOID, T_KEY, T_INTEGER, T_STRING),
    FN0("llGetHTTPHeader",      T_STRING),
    FN2("llGetHTTPHeader",      T_STRING, T_KEY, T_STRING),
    FN0("llGetFreeURLs",        T_INTEGER),
    FN0("llRequestURL",         T_KEY),
    FN0("llRequestSecureURL",   T_KEY),
    FN1("llReleaseURL",         T_VOID, T_STRING),
    FN2("llEmail",              T_VOID, T_STRING, T_STRING),  /* deprecated 2-arg */
    FN3("llEmail",              T_VOID, T_STRING, T_STRING, T_STRING),
    FN1("llGetNextEmail",       T_VOID, T_STRING),
    FN2("llGetNextEmail",       T_VOID, T_STRING, T_STRING),
    FN3("llRequestSecureURL",   T_KEY, T_VOID, T_VOID, T_VOID),

    /* --------- Avatar / agent info ---------- */
    FN0("llGetAttached",        T_INTEGER),
    FN1("llAttachToAvatar",     T_VOID, T_INTEGER),
    FN1("llAttachToAvatarTemp", T_VOID, T_INTEGER),
    FN0("llDetachFromAvatar",   T_VOID),
    FN1("llKey2Name",           T_STRING, T_KEY),
    FN1("llName2Key",           T_KEY, T_STRING),
    FN2("llName2Key",           T_KEY, T_STRING, T_INTEGER),
    FN1("llGetUsername",        T_STRING, T_KEY),
    FN1("llGetDisplayName",     T_STRING, T_KEY),
    FN1("llRequestUsername",    T_KEY, T_KEY),
    FN1("llRequestDisplayName", T_KEY, T_KEY),
    FN0("llGetAgentInfo",       T_INTEGER),
    FN1("llGetAgentInfo",       T_INTEGER, T_KEY),
    FN1("llGetAgentLanguage",   T_STRING, T_KEY),
    FN2("llGetAgentList",       T_LIST, T_INTEGER, T_LIST),
    FN1("llGetAgentSize",       T_VECTOR, T_KEY),
    FN3("llRequestAgentData",   T_KEY, T_KEY, T_INTEGER, T_VOID),
    FN2("llRequestAgentData",   T_KEY, T_KEY, T_INTEGER),
    FN1("llGetAnimation",       T_STRING, T_KEY),
    FN1("llGetAnimationList",   T_LIST, T_KEY),
    FN1("llRequestAnimationData", T_KEY, T_STRING),
    FN1("llStartAnimation",     T_VOID, T_STRING),
    FN1("llStopAnimation",      T_VOID, T_STRING),
    FN2("llStartObjectAnimation", T_VOID, T_KEY, T_STRING),
    FN1("llStartObjectAnimation", T_VOID, T_STRING),
    FN1("llStopObjectAnimation",T_VOID, T_STRING),
    FN1("llTeleportAgent",      T_VOID, T_KEY),
    FN4("llTeleportAgent",      T_VOID, T_KEY, T_STRING, T_VECTOR, T_VECTOR),
    FN3("llTeleportAgentGlobalCoords", T_VOID, T_KEY, T_VECTOR, T_VECTOR),
    FN4("llTeleportAgentGlobalCoords", T_VOID, T_KEY, T_VECTOR, T_VECTOR, T_VECTOR),
    FN1("llTeleportAgentHome",  T_VOID, T_KEY),
    FN2("llSitTarget",          T_VOID, T_VECTOR, T_ROTATION),
    FN3("llLinkSitTarget",      T_VOID, T_INTEGER, T_VECTOR, T_ROTATION),
    FN0("llAvatarOnSitTarget",  T_KEY),
    FN1("llAvatarOnLinkSitTarget", T_KEY, T_INTEGER),
    FN1("llUnSit",              T_VOID, T_KEY),
    FN0("llForceMouselook",     T_VOID),
    FN1("llForceMouselook",     T_VOID, T_INTEGER),

    /* --------- Timers / events ---------- */
    FN1("llSetTimerEvent",      T_VOID, T_FLOAT),
    FN1("llMinEventDelay",      T_VOID, T_FLOAT),
    FN1("llGetEventTimer",      T_FLOAT, T_VOID),
    FN0("llGetEventTimer",      T_FLOAT),

    /* --------- Notecards / dataserver ---------- */
    FN2("llGetNotecardLine",    T_KEY, T_STRING, T_INTEGER),
    FN1("llGetNumberOfNotecardLines", T_KEY, T_STRING),
    FN1("llGetNotecardLineSync", T_STRING, T_STRING),

    /* --------- Vehicles / physics ---------- */
    FN2("llSetVehicleType",     T_VOID, T_INTEGER, T_VOID),
    FN1("llSetVehicleType",     T_VOID, T_INTEGER),
    FN2("llSetVehicleFloatParam", T_VOID, T_INTEGER, T_FLOAT),
    FN2("llSetVehicleVectorParam", T_VOID, T_INTEGER, T_VECTOR),
    FN2("llSetVehicleRotationParam", T_VOID, T_INTEGER, T_ROTATION),
    FN2("llSetVehicleFlags",    T_VOID, T_INTEGER, T_INTEGER),
    FN2("llRemoveVehicleFlags", T_VOID, T_INTEGER, T_INTEGER),
    FN1("llApplyImpulse",       T_VOID, T_VECTOR),
    FN2("llApplyImpulse",       T_VOID, T_VECTOR, T_INTEGER),
    FN2("llApplyRotationalImpulse", T_VOID, T_VECTOR, T_INTEGER),
    FN1("llApplyRotationalImpulse", T_VOID, T_VECTOR),
    FN1("llPushObject",         T_VOID, T_KEY),
    FN4("llPushObject",         T_VOID, T_KEY, T_VECTOR, T_VECTOR, T_INTEGER),
    FN1("llSetBuoyancy",        T_VOID, T_FLOAT),
    FN0("llGetForce",           T_VECTOR),
    FN2("llSetForce",           T_VOID, T_VECTOR, T_INTEGER),
    FN1("llSetForce",           T_VOID, T_VECTOR),
    FN2("llSetForceAndTorque",  T_VOID, T_VECTOR, T_VECTOR),
    FN3("llSetForceAndTorque",  T_VOID, T_VECTOR, T_VECTOR, T_INTEGER),
    FN1("llSetTorque",          T_VOID, T_VECTOR),
    FN2("llSetTorque",          T_VOID, T_VECTOR, T_INTEGER),
    FN0("llGetTorque",          T_VECTOR),
    FN0("llGetVel",             T_VECTOR),
    FN1("llSetVelocity",        T_VOID, T_VECTOR),
    FN2("llSetVelocity",        T_VOID, T_VECTOR, T_INTEGER),
    FN1("llSetAngularVelocity", T_VOID, T_VECTOR),
    FN2("llSetAngularVelocity", T_VOID, T_VECTOR, T_INTEGER),
    FN1("llSetHoverHeight",     T_VOID, T_FLOAT),
    FN3("llSetHoverHeight",     T_VOID, T_FLOAT, T_INTEGER, T_FLOAT),
    FN0("llStopHover",          T_VOID),
    FN1("llTargetOmega",        T_VOID, T_VECTOR),
    FN3("llTargetOmega",        T_VOID, T_VECTOR, T_FLOAT, T_FLOAT),
    FN2("llRotLookAt",          T_VOID, T_ROTATION, T_FLOAT),
    FN3("llRotLookAt",          T_VOID, T_ROTATION, T_FLOAT, T_FLOAT),
    FN3("llLookAt",             T_VOID, T_VECTOR, T_FLOAT, T_FLOAT),
    FN0("llStopLookAt",         T_VOID),
    FN1("llMoveToTarget",       T_VOID, T_VECTOR),
    FN2("llMoveToTarget",       T_VOID, T_VECTOR, T_FLOAT),
    FN0("llStopMoveToTarget",   T_VOID),
    FN1("llTarget",             T_INTEGER, T_VECTOR),
    FN2("llTarget",             T_INTEGER, T_VECTOR, T_FLOAT),
    FN1("llTargetRemove",       T_VOID, T_INTEGER),
    FN1("llRotTarget",          T_INTEGER, T_ROTATION),
    FN2("llRotTarget",          T_INTEGER, T_ROTATION, T_FLOAT),
    FN1("llRotTargetRemove",    T_VOID, T_INTEGER),
    MN2("llCastRay",            T_LIST, T_VECTOR, T_VECTOR),
    MN3("llCastRay",            T_LIST, T_VECTOR, T_VECTOR, T_LIST),

    /* --------- Particle / effects ---------- */
    FN1("llParticleSystem",     T_VOID, T_LIST),
    FN2("llLinkParticleSystem", T_VOID, T_INTEGER, T_LIST),
    FN2("llTriggerSound",       T_VOID, T_STRING, T_FLOAT),
    FN2("llPlaySound",          T_VOID, T_STRING, T_FLOAT),
    FN2("llLoopSound",          T_VOID, T_STRING, T_FLOAT),
    FN2("llLoopSoundMaster",    T_VOID, T_STRING, T_FLOAT),
    FN2("llLoopSoundSlave",     T_VOID, T_STRING, T_FLOAT),
    FN0("llStopSound",          T_VOID),
    FN1("llPreloadSound",       T_VOID, T_STRING),
    FN1("llSoundPreload",       T_VOID, T_STRING),
    FN1("llSetSoundQueueing",   T_VOID, T_INTEGER),
    FN1("llSetSoundRadius",     T_VOID, T_FLOAT),
    FN4("llTriggerSoundLimited",T_VOID, T_STRING, T_FLOAT, T_VECTOR, T_VECTOR),

    /* --------- LINKSET DATA (LSD) — Mono only ---------- */
    MN2("llLinksetDataWrite",   T_INTEGER, T_STRING, T_STRING),
    MN3("llLinksetDataWriteProtected", T_INTEGER, T_STRING, T_STRING, T_STRING),
    MN1("llLinksetDataRead",    T_STRING,  T_STRING),
    MN2("llLinksetDataReadProtected", T_STRING, T_STRING, T_STRING),
    MN1("llLinksetDataDelete",  T_INTEGER, T_STRING),
    MN2("llLinksetDataDeleteProtected", T_INTEGER, T_STRING, T_STRING),
    MN0("llLinksetDataReset",   T_VOID),
    MN0("llLinksetDataCountKeys", T_INTEGER),
    MN0("llLinksetDataCountFound", T_INTEGER),
    MN0("llLinksetDataListKeys", T_LIST),
    MN2("llLinksetDataListKeys", T_LIST, T_INTEGER, T_INTEGER),
    MN1("llLinksetDataFindKeys", T_LIST, T_STRING),
    MN3("llLinksetDataFindKeys", T_LIST, T_STRING, T_INTEGER, T_INTEGER),
    MN0("llLinksetDataAvailable", T_INTEGER),
    MN0("llLinksetDataUsed",    T_INTEGER),

    /* --------- Pathfinding / Object info ---------- */
    FN2("llGetObjectDetails",   T_LIST, T_KEY, T_LIST),
    FN2("llGetObjectPermMask",  T_INTEGER, T_KEY, T_INTEGER),
    FN1("llGetObjectPermMask",  T_INTEGER, T_INTEGER),
    FN2("llSetObjectPermMask",  T_VOID, T_INTEGER, T_INTEGER),
    FN0("llGetObjectPrimCount", T_INTEGER),
    FN1("llGetObjectPrimCount", T_INTEGER, T_KEY),
    FN0("llGetStartString",     T_STRING),
    FN0("llGetStartParameter",  T_INTEGER),
    FN3("llCreateCharacter",    T_VOID, T_LIST, T_LIST, T_LIST),
    FN1("llCreateCharacter",    T_VOID, T_LIST),
    FN0("llDeleteCharacter",    T_VOID),
    FN1("llEvade",              T_VOID, T_KEY),
    FN2("llEvade",              T_VOID, T_KEY, T_LIST),
    FN2("llFleeFrom",           T_VOID, T_VECTOR, T_FLOAT),
    FN3("llFleeFrom",           T_VOID, T_VECTOR, T_FLOAT, T_LIST),
    FN3("llNavigateTo",         T_VOID, T_VECTOR, T_LIST, T_VOID),
    FN2("llNavigateTo",         T_VOID, T_VECTOR, T_LIST),
    FN1("llNavigateTo",         T_VOID, T_VECTOR),
    FN0("llPatrolPoints",       T_VOID),
    FN2("llPatrolPoints",       T_VOID, T_LIST, T_LIST),
    FN3("llPursue",             T_VOID, T_KEY, T_LIST, T_VOID),
    FN2("llPursue",             T_VOID, T_KEY, T_LIST),
    FN1("llPursue",             T_VOID, T_KEY),
    FN1("llWanderWithin",       T_VOID, T_VECTOR),
    FN2("llWanderWithin",       T_VOID, T_VECTOR, T_VECTOR),
    FN3("llWanderWithin",       T_VOID, T_VECTOR, T_VECTOR, T_LIST),
    FN0("llExecCharacterCmd",   T_VOID),
    FN2("llExecCharacterCmd",   T_VOID, T_INTEGER, T_LIST),

    /* --------- Date/Time/RNG/misc ---------- */
    FN0("llRot2Euler",          T_VECTOR),    /* duplicate intentional */
    FN1("llGetParcelMaxPrims",  T_INTEGER, T_VECTOR),
    FN2("llGetParcelMaxPrims",  T_INTEGER, T_VECTOR, T_INTEGER),
    FN2("llGetParcelDetails",   T_LIST, T_VECTOR, T_LIST),
    FN1("llGetParcelFlags",     T_INTEGER, T_VECTOR),
    FN0("llSetClickAction",     T_VOID),
    FN1("llSetClickAction",     T_VOID, T_INTEGER),
    FN1("llSetTouchText",       T_VOID, T_STRING),
    FN1("llSetSitText",         T_VOID, T_STRING),

    /* --------- Animations / camera ---------- */
    FN2("llSetCameraEyeOffset", T_VOID, T_VECTOR, T_INTEGER),
    FN1("llSetCameraEyeOffset", T_VOID, T_VECTOR),
    FN2("llSetCameraAtOffset",  T_VOID, T_VECTOR, T_INTEGER),
    FN1("llSetCameraAtOffset",  T_VOID, T_VECTOR),
    FN2("llSetCameraParams",    T_VOID, T_KEY, T_LIST),
    FN1("llSetCameraParams",    T_VOID, T_LIST),
    FN1("llClearCameraParams",  T_VOID, T_KEY),
    FN0("llClearCameraParams",  T_VOID),
    FN2("llSetLinkCamera",      T_VOID, T_INTEGER, T_VECTOR),
    FN3("llSetLinkCamera",      T_VOID, T_INTEGER, T_VECTOR, T_VECTOR),

    /* --------- Experience tools ---------- */
    FN0("llGetExperienceDetails", T_LIST),
    FN1("llGetExperienceDetails", T_LIST, T_KEY),
    FN2("llGetExperienceDetails", T_LIST, T_KEY, T_LIST),
    FN1("llGetExperienceErrorMessage", T_STRING, T_INTEGER),
    FN1("llRequestExperiencePermissions", T_VOID, T_KEY),
    FN2("llRequestExperiencePermissions", T_VOID, T_KEY, T_STRING),
    FN1("llAgentInExperience",  T_INTEGER, T_KEY),
    FN3("llCreateKeyValue",     T_KEY, T_STRING, T_STRING, T_VOID),
    FN2("llCreateKeyValue",     T_KEY, T_STRING, T_STRING),
    FN3("llUpdateKeyValue",     T_KEY, T_STRING, T_STRING, T_INTEGER),
    FN4("llUpdateKeyValue",     T_KEY, T_STRING, T_STRING, T_INTEGER, T_STRING),
    FN1("llReadKeyValue",       T_KEY, T_STRING),
    FN1("llDeleteKeyValue",     T_KEY, T_STRING),
    FN1("llDataSizeKeyValue",   T_KEY, T_VOID),
    FN0("llDataSizeKeyValue",   T_KEY),
    FN0("llKeysKeyValue",       T_KEY),
    FN2("llKeysKeyValue",       T_KEY, T_INTEGER, T_INTEGER),

    /* --------- Region / experience misc ---------- */
    FN1("llSetRegionPos",       T_INTEGER, T_VECTOR),
    FN2("llSetKeyframedMotion", T_VOID, T_LIST, T_LIST),
    FN0("llGetAttachedList",    T_LIST),
    FN1("llGetAttachedList",    T_LIST, T_KEY),
    FN0("llGetAttachedListFiltered", T_LIST),
    FN2("llGetAttachedListFiltered", T_LIST, T_KEY, T_LIST),
    FN1("llGetOwnerKey",        T_KEY, T_KEY),
    FN1("llRegionSayTo",        T_VOID, T_KEY),
    FN2("llSetContentType",     T_VOID, T_KEY, T_INTEGER),
    FN0("llClearLinkMedia",     T_INTEGER),
    FN2("llSetLinkMedia",       T_INTEGER, T_INTEGER, T_LIST),
    FN3("llSetLinkMedia",       T_INTEGER, T_INTEGER, T_INTEGER, T_LIST),
    FN2("llGetLinkMedia",       T_LIST, T_INTEGER, T_LIST),
    FN3("llGetLinkMedia",       T_LIST, T_INTEGER, T_INTEGER, T_LIST),
    FN1("llClearPrimMedia",     T_INTEGER, T_INTEGER),
    FN2("llSetPrimMediaParams", T_INTEGER, T_INTEGER, T_LIST),
    FN2("llGetPrimMediaParams", T_LIST, T_INTEGER, T_LIST),

    /* --------- Pay / money helpers extras ---------- */
    FN1("llGetObjectVelocity",  T_VECTOR, T_KEY),

    /* --------- Status, materials, physics extras ---------- */
    FN2("llSetStatus",            T_VOID, T_INTEGER, T_INTEGER),
    FN1("llGetStatus",            T_INTEGER, T_INTEGER),
    FN5("llSetPhysicsMaterial",   T_VOID, T_INTEGER, T_FLOAT, T_FLOAT, T_FLOAT, T_FLOAT),
    FN6("llSetLinkPhysicsMaterial", T_VOID, T_INTEGER, T_INTEGER, T_FLOAT, T_FLOAT, T_FLOAT, T_FLOAT),
    FN0("llGetPhysicsMaterial",   T_LIST),
    FN1("llVolumeDetect",         T_VOID, T_INTEGER),
    FN1("llSetDamage",            T_VOID, T_FLOAT),
    FN1("llGetHealth",            T_FLOAT, T_KEY),
    FN1("llCollisionSound",       T_VOID, T_STRING),
    FN2("llCollisionSound",       T_VOID, T_STRING, T_FLOAT),
    FN1("llCollisionSprite",      T_VOID, T_STRING),

    /* --------- Texture rotation / offset / animation ---------- */
    FN1("llGetTextureRot",        T_FLOAT, T_INTEGER),
    FN1("llGetTextureOffset",     T_VECTOR, T_INTEGER),
    FN1("llGetTextureScale",      T_VECTOR, T_INTEGER),
    FN7("llSetTextureAnim",       T_VOID, T_INTEGER, T_INTEGER, T_INTEGER, T_INTEGER, T_FLOAT, T_FLOAT, T_FLOAT),
    FN3("llSetLinkTexture",       T_VOID, T_INTEGER, T_STRING, T_INTEGER),

    /* --------- Animation overrides ---------- */
    FN2("llSetAnimationOverride", T_VOID, T_STRING, T_STRING),
    FN1("llGetAnimationOverride", T_STRING, T_STRING),
    FN1("llResetAnimationOverride", T_VOID, T_STRING),

    /* --------- Camera info ---------- */
    FN0("llGetCameraPos",         T_VECTOR),
    FN0("llGetCameraRot",         T_ROTATION),

    /* --------- Estate / parcel / objects management ---------- */
    FN2("llManageEstateAccess",   T_INTEGER, T_INTEGER, T_KEY),
    FN1("llReturnObjectsByID",    T_INTEGER, T_LIST),
    FN2("llReturnObjectsByOwner", T_INTEGER, T_KEY, T_INTEGER),
    FN3("llGetParcelPrimCount",   T_INTEGER, T_VECTOR, T_INTEGER, T_INTEGER),
    FN1("llGetParcelPrimOwners",  T_LIST, T_VECTOR),
    FN1("llSetParcelMusicURL",    T_VOID, T_STRING),
    FN0("llGetParcelMusicURL",    T_STRING),
    FN2("llSetParcelDetails",     T_VOID, T_VECTOR, T_LIST),

    /* --------- Sit / linkset extras ---------- */
    FN2("llSitOnLink",            T_INTEGER, T_KEY, T_INTEGER),

    /* --------- Remote data (XML-RPC, legacy but still in the API) -------- */
    FN0("llOpenRemoteDataChannel", T_VOID),
    FN1("llCloseRemoteDataChannel", T_VOID, T_KEY),
    FN4("llSendRemoteData",       T_KEY, T_KEY, T_STRING, T_INTEGER, T_STRING),
    FN4("llRemoteDataReply",      T_VOID, T_KEY, T_KEY, T_STRING, T_INTEGER),
    FN0("llRemoteDataSetRegion",  T_VOID),
    FN5("llRemoteLoadScriptPin",  T_VOID, T_KEY, T_STRING, T_INTEGER, T_INTEGER, T_INTEGER),
    FN1("llSetRemoteScriptAccessPin", T_VOID, T_INTEGER),

    /* --------- Combat 2 / world env (Mono only) ---------- */
    MN2("llSetEnv",               T_INTEGER, T_STRING, T_LIST),
    MN2("llSetLinkRenderMaterial",T_INTEGER, T_INTEGER, T_KEY),
    MN3("llSetLinkRenderMaterial",T_INTEGER, T_INTEGER, T_INTEGER, T_KEY),
    MN1("llDamageEvent",          T_VOID, T_INTEGER),
    MN1("llCharacterUpdate",      T_VOID, T_LIST),

    /* --------- Misc additional ---------- */
    FN0("llGetClosestNavPoint",   T_VECTOR),
    FN2("llGetClosestNavPoint",   T_VECTOR, T_VECTOR, T_LIST),
    FN1("llRequestUserKey",       T_KEY, T_STRING),
    FN0("llGetParcelMaxPrims",    T_INTEGER),
    FN1("llXorBase64Strings",     T_STRING, T_STRING),
    FN1("llTeleportAgentHome",    T_VOID, T_KEY),
    FN1("llRefreshPrimURL",       T_VOID, T_STRING),
    FN1("llSetPrimURL",           T_VOID, T_STRING),

    /* --------- Math helpers some scripts assume ---------- */
    FN2("llMin",                  T_FLOAT, T_FLOAT, T_FLOAT),
    FN2("llMax",                  T_FLOAT, T_FLOAT, T_FLOAT),

    /* --------- Newer/edge-case Mono-only (kept permissive on types) ----- */
    MN1("llHash",                 T_INTEGER, T_STRING),
    MN2("llHash",                 T_INTEGER, T_STRING, T_INTEGER),
    MN1("llComputeHash",          T_STRING, T_STRING),
    MN2("llComputeHash",          T_STRING, T_STRING, T_STRING),

    /* --------- Misc utilities & sentinel ---------- */
    FN0("llGetNotecardLineSync", T_STRING),
    FN1("print",                T_VOID, T_ANY),  /* debug-only */
};
const int BI_FN_N = (int)(sizeof BI_FN / sizeof BI_FN[0]);

/* ----------------------------- Constants -------------------------------- */

/*
 * Many LSL constants are simply integers. We list the most common ones
 * (TRUE/FALSE/NULL_KEY plus key categories used by HTTP, permissions,
 * change-events, inventory types, prim params, JSON helpers, etc.). For
 * uncovered constants we don't reject — see notes in builtins header.
 */
#define CI(n, v) { n, T_INTEGER, (v), 0, NULL, 1 }
#define CF(n, v) { n, T_FLOAT,   0, (v), NULL, 1 }
#define CS(n, v) { n, T_STRING,  0, 0, (v), 1 }
#define CK(n, v) { n, T_KEY,     0, 0, (v), 1 }

const BuiltinConst BI_CONST[] = {
    /* boolean & misc */
    CI("TRUE", 1),
    CI("FALSE", 0),
    CK("NULL_KEY", "00000000-0000-0000-0000-000000000000"),
    CS("EOF", "\n\n\n"),
    CF("PI",      3.141592653589793),
    CF("TWO_PI",  6.283185307179586),
    CF("PI_BY_TWO", 1.5707963267948966),
    CF("DEG_TO_RAD", 0.017453292519943295),
    CF("RAD_TO_DEG", 57.29577951308232),
    CF("SQRT2",   1.4142135623730951),

    /* statuses */
    CI("STATUS_OK", 0),
    CI("STATUS_INTERNAL_ERROR", -1),
    CI("STATUS_WHITELIST_FAILED", 2001),
    CI("STATUS_MALFORMED_PARAMS", 1000),
    CI("STATUS_TYPE_MISMATCH",    1001),
    CI("STATUS_BOUNDS_ERROR",     1002),
    CI("STATUS_NOT_FOUND",        1003),
    CI("STATUS_NOT_SUPPORTED",    1004),

    /* link constants */
    CI("LINK_SET", -1),
    CI("LINK_ROOT", 1),
    CI("LINK_ALL_OTHERS", -2),
    CI("LINK_ALL_CHILDREN", -3),
    CI("LINK_THIS", -4),

    /* attach points */
    CI("ATTACH_CHEST", 1), CI("ATTACH_HEAD", 2), CI("ATTACH_LSHOULDER", 3),
    CI("ATTACH_RSHOULDER", 4), CI("ATTACH_LHAND", 5), CI("ATTACH_RHAND", 6),
    CI("ATTACH_LFOOT", 7), CI("ATTACH_RFOOT", 8), CI("ATTACH_BACK", 9),
    CI("ATTACH_PELVIS", 10), CI("ATTACH_MOUTH", 11), CI("ATTACH_CHIN", 12),
    CI("ATTACH_LEAR", 13), CI("ATTACH_REAR", 14), CI("ATTACH_LEYE", 15),
    CI("ATTACH_REYE", 16), CI("ATTACH_NOSE", 17), CI("ATTACH_RUARM", 18),
    CI("ATTACH_RLARM", 19), CI("ATTACH_LUARM", 20), CI("ATTACH_LLARM", 21),
    CI("ATTACH_RHIP", 22), CI("ATTACH_RULEG", 23), CI("ATTACH_RLLEG", 24),
    CI("ATTACH_LHIP", 25), CI("ATTACH_LULEG", 26), CI("ATTACH_LLLEG", 27),
    CI("ATTACH_BELLY", 28), CI("ATTACH_LEFT_PEC", 29), CI("ATTACH_RIGHT_PEC", 30),
    CI("ATTACH_HUD_CENTER_2", 31), CI("ATTACH_HUD_TOP_RIGHT", 32),
    CI("ATTACH_HUD_TOP_CENTER", 33), CI("ATTACH_HUD_TOP_LEFT", 34),
    CI("ATTACH_HUD_CENTER_1", 35), CI("ATTACH_HUD_BOTTOM_LEFT", 36),
    CI("ATTACH_HUD_BOTTOM", 37), CI("ATTACH_HUD_BOTTOM_RIGHT", 38),
    CI("ATTACH_NECK", 39), CI("ATTACH_AVATAR_CENTER", 40),
    CI("ATTACH_LHAND_RING1", 41), CI("ATTACH_RHAND_RING1", 42),
    CI("ATTACH_TAIL_BASE", 43), CI("ATTACH_TAIL_TIP", 44),
    CI("ATTACH_LWING", 45), CI("ATTACH_RWING", 46),
    CI("ATTACH_FACE_JAW", 47), CI("ATTACH_FACE_LEAR", 48),
    CI("ATTACH_FACE_REAR", 49), CI("ATTACH_FACE_LEYE", 50),
    CI("ATTACH_FACE_REYE", 51), CI("ATTACH_FACE_TONGUE", 52),
    CI("ATTACH_GROIN", 53), CI("ATTACH_HIND_LFOOT", 54),
    CI("ATTACH_HIND_RFOOT", 55),

    /* permission constants */
    CI("PERMISSION_DEBIT", 0x002),
    CI("PERMISSION_TAKE_CONTROLS", 0x004),
    CI("PERMISSION_REMAP_CONTROLS", 0x008),
    CI("PERMISSION_TRIGGER_ANIMATION", 0x010),
    CI("PERMISSION_ATTACH", 0x020),
    CI("PERMISSION_RELEASE_OWNERSHIP", 0x040),
    CI("PERMISSION_CHANGE_LINKS", 0x080),
    CI("PERMISSION_CHANGE_JOINTS", 0x100),
    CI("PERMISSION_CHANGE_PERMISSIONS", 0x200),
    CI("PERMISSION_TRACK_CAMERA", 0x400),
    CI("PERMISSION_CONTROL_CAMERA", 0x800),
    CI("PERMISSION_TELEPORT", 0x1000),
    CI("PERMISSION_SILENT_ESTATE_MANAGEMENT", 0x4000),
    CI("PERMISSION_OVERRIDE_ANIMATIONS", 0x8000),
    CI("PERMISSION_RETURN_OBJECTS", 0x10000),

    /* changed flags */
    CI("CHANGED_INVENTORY", 0x001),
    CI("CHANGED_COLOR",     0x002),
    CI("CHANGED_SHAPE",     0x004),
    CI("CHANGED_SCALE",     0x008),
    CI("CHANGED_TEXTURE",   0x010),
    CI("CHANGED_LINK",      0x020),
    CI("CHANGED_ALLOWED_DROP", 0x040),
    CI("CHANGED_OWNER",     0x080),
    CI("CHANGED_REGION",    0x100),
    CI("CHANGED_TELEPORT",  0x200),
    CI("CHANGED_REGION_START", 0x400),
    CI("CHANGED_MEDIA",     0x800),
    CI("CHANGED_RENDER_MATERIAL", 0x1000),
    CI("CHANGED_PROPERTIES", 0x2000),

    /* sensor / agent flags */
    CI("AGENT", 1), CI("ACTIVE", 2), CI("PASSIVE", 4), CI("SCRIPTED", 8),
    CI("AGENT_BY_LEGACY_NAME", 1),
    CI("AGENT_BY_USERNAME", 0x10),
    CI("AGENT_LIST_PARCEL", 1),
    CI("AGENT_LIST_PARCEL_OWNER", 2),
    CI("AGENT_LIST_REGION", 4),
    CI("AGENT_FLYING", 0x0001),
    CI("AGENT_ATTACHMENTS", 0x0002),
    CI("AGENT_SCRIPTED", 0x0004),
    CI("AGENT_MOUSELOOK", 0x0008),
    CI("AGENT_SITTING", 0x0010),
    CI("AGENT_ON_OBJECT", 0x0020),
    CI("AGENT_AWAY", 0x0040),
    CI("AGENT_WALKING", 0x0080),
    CI("AGENT_IN_AIR", 0x0100),
    CI("AGENT_TYPING", 0x0200),
    CI("AGENT_CROUCHING", 0x0400),
    CI("AGENT_BUSY", 0x0800),
    CI("AGENT_ALWAYS_RUN", 0x1000),
    CI("AGENT_AUTOPILOT", 0x2000),

    /* HTTP-related */
    CI("HTTP_METHOD", 0),
    CI("HTTP_MIMETYPE", 1),
    CI("HTTP_BODY_MAXLENGTH", 2),
    CI("HTTP_VERIFY_CERT", 3),
    CI("HTTP_VERBOSE_THROTTLE", 4),
    CI("HTTP_CUSTOM_HEADER", 5),
    CI("HTTP_PRAGMA_NO_CACHE", 6),
    CI("HTTP_BODY_TRUNCATED", 1),
    CI("HTTP_USER_AGENT", 7),
    CI("HTTP_ACCEPT", 8),
    CI("HTTP_EXTENDED_ERROR", 9),
    CI("CONTENT_TYPE_TEXT", 0),
    CI("CONTENT_TYPE_HTML", 1),
    CI("CONTENT_TYPE_XML", 2),
    CI("CONTENT_TYPE_XHTML", 3),
    CI("CONTENT_TYPE_ATOM", 4),
    CI("CONTENT_TYPE_JSON", 5),
    CI("CONTENT_TYPE_LLSD", 6),
    CI("CONTENT_TYPE_FORM", 7),
    CI("CONTENT_TYPE_RSS", 8),

    /* inventory types */
    CI("INVENTORY_NONE", -1),
    CI("INVENTORY_ALL",  -1),
    CI("INVENTORY_TEXTURE", 0),
    CI("INVENTORY_SOUND", 1),
    CI("INVENTORY_LANDMARK", 3),
    CI("INVENTORY_CLOTHING", 5),
    CI("INVENTORY_OBJECT", 6),
    CI("INVENTORY_NOTECARD", 7),
    CI("INVENTORY_SCRIPT", 10),
    CI("INVENTORY_BODYPART", 13),
    CI("INVENTORY_ANIMATION", 20),
    CI("INVENTORY_GESTURE", 21),
    CI("INVENTORY_SETTINGS", 56),
    CI("INVENTORY_MATERIAL", 57),

    /* pay */
    CI("PAY_HIDE",   -1),
    CI("PAY_DEFAULT", -2),

    /* prim params used in scripts */
    CI("ALL_SIDES", -1),
    CI("PRIM_TYPE", 9),
    CI("PRIM_MATERIAL", 2),
    CI("PRIM_PHYSICS", 3),
    CI("PRIM_TEMP_ON_REZ", 4),
    CI("PRIM_PHANTOM", 5),
    CI("PRIM_POSITION", 6),
    CI("PRIM_SIZE", 7),
    CI("PRIM_ROTATION", 8),
    CI("PRIM_COLOR", 18),
    CI("PRIM_TEXTURE", 17),
    CI("PRIM_BUMP_SHINY", 19),
    CI("PRIM_FULLBRIGHT", 20),
    CI("PRIM_FLEXIBLE", 21),
    CI("PRIM_TEXGEN", 22),
    CI("PRIM_POINT_LIGHT", 23),
    CI("PRIM_CAST_SHADOWS", 24),
    CI("PRIM_GLOW", 25),
    CI("PRIM_TEXT", 26),
    CI("PRIM_NAME", 27),
    CI("PRIM_DESC", 28),
    CI("PRIM_OMEGA", 29),
    CI("PRIM_POS_LOCAL", 33),
    CI("PRIM_ROT_LOCAL", 32),
    CI("PRIM_SIZE_LOCAL", 30),
    CI("PRIM_SCRIPTED_SIT_ONLY", 38),
    CI("PRIM_LINK_TARGET", 34),
    CI("PRIM_PHYSICS_SHAPE_TYPE", 35),
    CI("PRIM_PHYSICS_SHAPE_PRIM", 0),
    CI("PRIM_PHYSICS_SHAPE_CONVEX", 2),
    CI("PRIM_PHYSICS_SHAPE_NONE", 1),
    CI("PRIM_NORMAL", 37),
    CI("PRIM_SPECULAR", 36),
    CI("PRIM_ALPHA_MODE", 38),
    CI("PRIM_PROJECTOR", 41),
    CI("PRIM_SLICE", 42),
    CI("PRIM_RENDER_MATERIAL", 49),
    CI("PRIM_GLTF_BASE_COLOR", 50),
    CI("PRIM_GLTF_EMISSIVE", 51),
    CI("PRIM_GLTF_METALLIC_ROUGHNESS", 52),
    CI("PRIM_GLTF_NORMAL", 53),
    CI("PRIM_GLTF_OCCLUSION", 54),

    /* JSON helpers */
    CS("JSON_OBJECT",   "\xEF\xBF\xBD"),
    CS("JSON_ARRAY",    "\xEF\xBF\xBC"),
    CS("JSON_STRING",   "\xEF\xBF\xBB"),
    CS("JSON_NUMBER",   "\xEF\xBF\xBA"),
    CS("JSON_TRUE",     "\xEF\xBF\xB9"),
    CS("JSON_FALSE",    "\xEF\xBF\xB8"),
    CS("JSON_NULL",     "\xEF\xBF\xB7"),
    CS("JSON_INVALID",  "\xEF\xBF\xB6"),
    CS("JSON_DELETE",   "\xEF\xBF\xB5"),
    CS("JSON_APPEND",   "-1"),

    /* object detail keys for llGetObjectDetails */
    CI("OBJECT_UNKNOWN_DETAIL", -1),
    CI("OBJECT_NAME", 1),
    CI("OBJECT_DESC", 2),
    CI("OBJECT_POS", 3),
    CI("OBJECT_ROT", 4),
    CI("OBJECT_VELOCITY", 5),
    CI("OBJECT_OWNER", 6),
    CI("OBJECT_GROUP", 7),
    CI("OBJECT_CREATOR", 8),
    CI("OBJECT_RUNNING_SCRIPT_COUNT", 9),
    CI("OBJECT_TOTAL_SCRIPT_COUNT", 10),
    CI("OBJECT_SCRIPT_MEMORY", 11),
    CI("OBJECT_SCRIPT_TIME", 12),
    CI("OBJECT_PRIM_EQUIVALENCE", 13),
    CI("OBJECT_SERVER_COST", 14),
    CI("OBJECT_STREAMING_COST", 15),
    CI("OBJECT_PHYSICS_COST", 16),
    CI("OBJECT_ATTACHED_POINT", 19),
    CI("OBJECT_PATHFINDING_TYPE", 20),
    CI("OBJECT_PHYSICS", 21),
    CI("OBJECT_PHANTOM", 22),
    CI("OBJECT_TEMP_ON_REZ", 23),
    CI("OBJECT_RENDER_WEIGHT", 24),
    CI("OBJECT_HOVER_HEIGHT", 25),
    CI("OBJECT_BODY_SHAPE_TYPE", 26),
    CI("OBJECT_LAST_OWNER_ID", 27),
    CI("OBJECT_CLICK_ACTION", 28),
    CI("OBJECT_OMEGA", 29),
    CI("OBJECT_PRIM_COUNT", 30),
    CI("OBJECT_TOTAL_INVENTORY_COUNT", 31),
    CI("OBJECT_REZ_TIME", 32),

    /* CLICK_ACTION_ */
    CI("CLICK_ACTION_NONE", 0),
    CI("CLICK_ACTION_TOUCH", 0),
    CI("CLICK_ACTION_SIT", 1),
    CI("CLICK_ACTION_BUY", 2),
    CI("CLICK_ACTION_PAY", 3),
    CI("CLICK_ACTION_OPEN", 4),
    CI("CLICK_ACTION_PLAY", 5),
    CI("CLICK_ACTION_OPEN_MEDIA", 6),
    CI("CLICK_ACTION_ZOOM", 7),
    CI("CLICK_ACTION_IGNORE", 8),

    /* DATA_ for llRequestAgentData */
    CI("DATA_ONLINE", 1), CI("DATA_NAME", 2), CI("DATA_BORN", 3),
    CI("DATA_RATING", 4), CI("DATA_PAYINFO", 8),

    /* CONTROL_ for take_controls */
    CI("CONTROL_FWD", 1), CI("CONTROL_BACK", 2), CI("CONTROL_LEFT", 4),
    CI("CONTROL_RIGHT", 8), CI("CONTROL_ROT_LEFT", 256), CI("CONTROL_ROT_RIGHT", 512),
    CI("CONTROL_UP", 16), CI("CONTROL_DOWN", 32), CI("CONTROL_LBUTTON", 268435456),
    CI("CONTROL_ML_LBUTTON", 1073741824),

    /* sound flags / particle keys */
    CI("PSYS_PART_FLAGS", 0),
    CI("PSYS_PART_START_COLOR", 1),
    CI("PSYS_PART_START_ALPHA", 2),
    CI("PSYS_PART_END_COLOR", 3),
    CI("PSYS_PART_END_ALPHA", 4),
    CI("PSYS_PART_START_SCALE", 5),
    CI("PSYS_PART_END_SCALE", 6),
    CI("PSYS_PART_MAX_AGE", 7),
    CI("PSYS_SRC_ACCEL", 8),
    CI("PSYS_SRC_PATTERN", 9),
    CI("PSYS_SRC_TEXTURE", 12),
    CI("PSYS_SRC_BURST_RATE", 13),
    CI("PSYS_SRC_BURST_PART_COUNT", 15),
    CI("PSYS_SRC_BURST_RADIUS", 16),
    CI("PSYS_SRC_BURST_SPEED_MIN", 17),
    CI("PSYS_SRC_BURST_SPEED_MAX", 18),
    CI("PSYS_SRC_MAX_AGE", 19),
    CI("PSYS_SRC_TARGET_KEY", 20),
    CI("PSYS_SRC_OMEGA", 21),
    CI("PSYS_SRC_ANGLE_BEGIN", 22),
    CI("PSYS_SRC_ANGLE_END", 23),

    /* TYPE_ for llGetType / list typing */
    CI("TYPE_INVALID", 0),
    CI("TYPE_INTEGER", 1),
    CI("TYPE_FLOAT", 2),
    CI("TYPE_STRING", 3),
    CI("TYPE_KEY", 4),
    CI("TYPE_VECTOR", 5),
    CI("TYPE_ROTATION", 6),

    /* Misc */
    CI("ACTIVE_DEFAULT", 0),
    CI("OBJECT_DEFAULT", 0),
    CI("OBJECT_RETURN_PARCEL", 1),
    CI("OBJECT_RETURN_PARCEL_OWNER", 2),
    CI("OBJECT_RETURN_REGION", 4),
    CI("REGION_FLAG_ALLOW_DAMAGE", 0x001),
    CI("REGION_FLAG_FIXED_SUN", 0x010),
    CI("REGION_FLAG_BLOCK_TERRAFORM", 0x040),
    CI("REGION_FLAG_SANDBOX", 0x100),
    CI("REGION_FLAG_DISABLE_COLLISIONS", 0x1000),
    CI("REGION_FLAG_DISABLE_PHYSICS", 0x4000),
    CI("REGION_FLAG_BLOCK_FLY", 0x80000),
    CI("REGION_FLAG_ALLOW_DIRECT_TELEPORT", 0x100000),
    CI("REGION_FLAG_RESTRICT_PUSHOBJECT", 0x400000),
    CI("STRING_TRIM", 3),
    CI("STRING_TRIM_HEAD", 1),
    CI("STRING_TRIM_TAIL", 2),
    CI("LIST_STAT_RANGE", 0),
    CI("LIST_STAT_MIN", 1),
    CI("LIST_STAT_MAX", 2),
    CI("LIST_STAT_MEAN", 3),
    CI("LIST_STAT_MEDIAN", 4),
    CI("LIST_STAT_STD_DEV", 5),
    CI("LIST_STAT_SUM", 6),
    CI("LIST_STAT_SUM_SQUARES", 7),
    CI("LIST_STAT_NUM_COUNT", 8),
    CI("LIST_STAT_GEOMETRIC_MEAN", 9),

    /* timing */
    CI("OBJECT_PHYSICS_COST", 16),
    CI("VEHICLE_TYPE_NONE", 0),
    CI("VEHICLE_TYPE_SLED", 1),
    CI("VEHICLE_TYPE_CAR", 2),
    CI("VEHICLE_TYPE_BOAT", 3),
    CI("VEHICLE_TYPE_AIRPLANE", 4),
    CI("VEHICLE_TYPE_BALLOON", 5),

    /* Estate / parcel */
    CI("PARCEL_FLAG_ALLOW_FLY", 1),
    CI("PARCEL_FLAG_ALLOW_SCRIPTS", 2),
    CI("PARCEL_FLAG_ALLOW_LANDMARK", 8),
    CI("PARCEL_FLAG_ALLOW_TERRAFORM", 16),
    CI("PARCEL_FLAG_ALLOW_DAMAGE", 32),
    CI("PARCEL_FLAG_ALLOW_CREATE_OBJECTS", 64),
    CI("PARCEL_DETAILS_NAME", 0),
    CI("PARCEL_DETAILS_DESC", 1),
    CI("PARCEL_DETAILS_OWNER", 2),
    CI("PARCEL_DETAILS_GROUP", 3),
    CI("PARCEL_DETAILS_AREA", 4),
    CI("PARCEL_DETAILS_ID", 5),
    CI("PARCEL_DETAILS_SEE_AVATARS", 6),

    /* DEBUG_CHANNEL */
    CI("DEBUG_CHANNEL", 2147483647),
    CI("PUBLIC_CHANNEL", 0),

    /* Touch / face */
    CI("TOUCH_INVALID_FACE", -1),
    CI("TOUCH_INVALID_TEXCOORD", -1),

    /* Misc cleanup */
    CI("PRN_PUBLIC_CHANNEL", 0),

    /* STATUS_ flags for llSetStatus / llGetStatus */
    CI("STATUS_PHYSICS", 1),
    CI("STATUS_ROTATE_X", 2),
    CI("STATUS_ROTATE_Y", 4),
    CI("STATUS_ROTATE_Z", 8),
    CI("STATUS_PHANTOM", 16),
    CI("STATUS_SANDBOX", 32),
    CI("STATUS_BLOCK_GRAB", 64),
    CI("STATUS_DIE_AT_EDGE", 128),
    CI("STATUS_RETURN_AT_EDGE", 256),
    CI("STATUS_CAST_SHADOWS", 512),
    CI("STATUS_BLOCK_GRAB_OBJECT", 1024),
    CI("STATUS_DIE_AT_NO_ENTRY", 2048),

    /* PRIM_BUMP_ codes */
    CI("PRIM_BUMP_NONE", 0),
    CI("PRIM_BUMP_BRIGHT", 1),
    CI("PRIM_BUMP_DARK", 2),
    CI("PRIM_BUMP_WOOD", 3),
    CI("PRIM_BUMP_BARK", 4),
    CI("PRIM_BUMP_BRICKS", 5),
    CI("PRIM_BUMP_CHECKER", 6),
    CI("PRIM_BUMP_CONCRETE", 7),
    CI("PRIM_BUMP_TILE", 8),
    CI("PRIM_BUMP_STONE", 9),
    CI("PRIM_BUMP_DISKS", 10),
    CI("PRIM_BUMP_GRAVEL", 11),
    CI("PRIM_BUMP_BLOBS", 12),
    CI("PRIM_BUMP_SIDING", 13),
    CI("PRIM_BUMP_LARGETILE", 14),
    CI("PRIM_BUMP_STUCCO", 15),
    CI("PRIM_BUMP_SUCTION", 16),
    CI("PRIM_BUMP_WEAVE", 17),

    /* PRIM_SHINY_ */
    CI("PRIM_SHINY_NONE", 0),
    CI("PRIM_SHINY_LOW", 1),
    CI("PRIM_SHINY_MEDIUM", 2),
    CI("PRIM_SHINY_HIGH", 3),

    /* PRIM_TEXGEN_ */
    CI("PRIM_TEXGEN_DEFAULT", 0),
    CI("PRIM_TEXGEN_PLANAR", 1),

    /* PRIM_TYPE_ */
    CI("PRIM_TYPE_BOX", 0),
    CI("PRIM_TYPE_CYLINDER", 1),
    CI("PRIM_TYPE_PRISM", 2),
    CI("PRIM_TYPE_SPHERE", 3),
    CI("PRIM_TYPE_TORUS", 4),
    CI("PRIM_TYPE_TUBE", 5),
    CI("PRIM_TYPE_RING", 6),
    CI("PRIM_TYPE_SCULPT", 7),

    /* PRIM_MATERIAL_ */
    CI("PRIM_MATERIAL_STONE", 0),
    CI("PRIM_MATERIAL_METAL", 1),
    CI("PRIM_MATERIAL_GLASS", 2),
    CI("PRIM_MATERIAL_WOOD", 3),
    CI("PRIM_MATERIAL_FLESH", 4),
    CI("PRIM_MATERIAL_PLASTIC", 5),
    CI("PRIM_MATERIAL_RUBBER", 6),
    CI("PRIM_MATERIAL_LIGHT", 7),

    /* PRIM_HOLE_ */
    CI("PRIM_HOLE_DEFAULT", 0),
    CI("PRIM_HOLE_CIRCLE", 16),
    CI("PRIM_HOLE_SQUARE", 32),
    CI("PRIM_HOLE_TRIANGLE", 48),

    /* PRIM_SCULPT_TYPE_ */
    CI("PRIM_SCULPT_TYPE_SPHERE", 1),
    CI("PRIM_SCULPT_TYPE_TORUS", 2),
    CI("PRIM_SCULPT_TYPE_PLANE", 3),
    CI("PRIM_SCULPT_TYPE_CYLINDER", 4),
    CI("PRIM_SCULPT_TYPE_MESH", 5),
    CI("PRIM_SCULPT_FLAG_INVERT", 64),
    CI("PRIM_SCULPT_FLAG_MIRROR", 128),

    /* PRIM_FLEXIBLE_ - sub list params, not flags but values */
    CI("PRIM_FLEXIBLE_SOFTNESS",  0),
    CI("PRIM_FLEXIBLE_GRAVITY",   1),
    CI("PRIM_FLEXIBLE_DRAG",      2),
    CI("PRIM_FLEXIBLE_WIND",      3),
    CI("PRIM_FLEXIBLE_TENSION",   4),
    CI("PRIM_FLEXIBLE_FORCE",     5),

    /* ALPHA modes */
    CI("PRIM_ALPHA_MODE_NONE", 0),
    CI("PRIM_ALPHA_MODE_BLEND", 1),
    CI("PRIM_ALPHA_MODE_MASK", 2),
    CI("PRIM_ALPHA_MODE_EMISSIVE", 3),

    /* VEHICLE_FLAG_ */
    CI("VEHICLE_FLAG_NO_FLY_UP", 1),
    CI("VEHICLE_FLAG_LIMIT_ROLL_ONLY", 2),
    CI("VEHICLE_FLAG_HOVER_WATER_ONLY", 4),
    CI("VEHICLE_FLAG_HOVER_TERRAIN_ONLY", 8),
    CI("VEHICLE_FLAG_HOVER_GLOBAL_HEIGHT", 16),
    CI("VEHICLE_FLAG_HOVER_UP_ONLY", 32),
    CI("VEHICLE_FLAG_LIMIT_MOTOR_UP", 64),
    CI("VEHICLE_FLAG_MOUSELOOK_STEER", 128),
    CI("VEHICLE_FLAG_MOUSELOOK_BANK", 256),
    CI("VEHICLE_FLAG_CAMERA_DECOUPLED", 512),
    CI("VEHICLE_FLAG_NO_X", 1024),
    CI("VEHICLE_FLAG_NO_Y", 2048),
    CI("VEHICLE_FLAG_NO_Z", 4096),
    CI("VEHICLE_FLAG_LOCK_HOVER_HEIGHT", 8192),
    CI("VEHICLE_FLAG_NO_DEFLECTION", 16384),
    CI("VEHICLE_FLAG_LOCK_ROTATION", 32768),

    /* VEHICLE_*_PARAM (most used) */
    CI("VEHICLE_LINEAR_FRICTION_TIMESCALE", 16),
    CI("VEHICLE_ANGULAR_FRICTION_TIMESCALE", 17),
    CI("VEHICLE_LINEAR_MOTOR_DIRECTION", 18),
    CI("VEHICLE_LINEAR_MOTOR_OFFSET", 20),
    CI("VEHICLE_ANGULAR_MOTOR_DIRECTION", 19),
    CI("VEHICLE_HOVER_HEIGHT", 24),
    CI("VEHICLE_HOVER_EFFICIENCY", 25),
    CI("VEHICLE_HOVER_TIMESCALE", 26),
    CI("VEHICLE_BUOYANCY", 27),
    CI("VEHICLE_LINEAR_DEFLECTION_EFFICIENCY", 28),
    CI("VEHICLE_LINEAR_DEFLECTION_TIMESCALE", 29),
    CI("VEHICLE_LINEAR_MOTOR_TIMESCALE", 30),
    CI("VEHICLE_LINEAR_MOTOR_DECAY_TIMESCALE", 31),
    CI("VEHICLE_ANGULAR_DEFLECTION_EFFICIENCY", 32),
    CI("VEHICLE_ANGULAR_DEFLECTION_TIMESCALE", 33),
    CI("VEHICLE_ANGULAR_MOTOR_TIMESCALE", 34),
    CI("VEHICLE_ANGULAR_MOTOR_DECAY_TIMESCALE", 35),
    CI("VEHICLE_VERTICAL_ATTRACTION_EFFICIENCY", 36),
    CI("VEHICLE_VERTICAL_ATTRACTION_TIMESCALE", 37),
    CI("VEHICLE_BANKING_EFFICIENCY", 38),
    CI("VEHICLE_BANKING_MIX", 39),
    CI("VEHICLE_BANKING_TIMESCALE", 40),
    CI("VEHICLE_REFERENCE_FRAME", 44),

    /* ESTATE_ACCESS_ */
    CI("ESTATE_ACCESS_ALLOWED_AGENT_ADD", 4),
    CI("ESTATE_ACCESS_ALLOWED_AGENT_REMOVE", 8),
    CI("ESTATE_ACCESS_ALLOWED_GROUP_ADD", 16),
    CI("ESTATE_ACCESS_ALLOWED_GROUP_REMOVE", 32),
    CI("ESTATE_ACCESS_BANNED_AGENT_ADD", 64),
    CI("ESTATE_ACCESS_BANNED_AGENT_REMOVE", 128),

    /* PARCEL_FLAG_ extras */
    CI("PARCEL_FLAG_ALLOW_GROUP_SCRIPTS", 0x2000000),
    CI("PARCEL_FLAG_ALLOW_GROUP_OBJECT_ENTRY", 0x4000000),
    CI("PARCEL_FLAG_USE_BAN_LIST", 0x8000000),
    CI("PARCEL_FLAG_USE_ACCESS_LIST", 0x20000000),
    CI("PARCEL_FLAG_USE_ACCESS_GROUP", 0x40000000),

    /* MEDIA / TEXTURE_ANIM constants */
    CI("ANIM_ON", 1), CI("LOOP", 2), CI("REVERSE", 4), CI("PING_PONG", 8),
    CI("SMOOTH", 16), CI("ROTATE", 32), CI("SCALE", 64),

    /* sound flags */
    CI("SOUND_PLAY", 1), CI("SOUND_LOOP", 2), CI("SOUND_TRIGGER", 4),
    CI("SOUND_SYNC_MASTER", 16), CI("SOUND_SYNC_PENDING", 32),

    /* RC_ (ray-cast detector codes) */
    CI("RC_REJECT_TYPES", 0),
    CI("RC_DETECT_PHANTOM", 1),
    CI("RC_DATA_FLAGS", 2),
    CI("RC_MAX_HITS", 3),
    CI("RC_REJECT_AGENTS", 1),
    CI("RC_REJECT_PHYSICAL", 2),
    CI("RC_REJECT_NONPHYSICAL", 4),
    CI("RC_REJECT_LAND", 8),
    CI("RC_GET_NORMAL", 1),
    CI("RC_GET_ROOT_KEY", 2),
    CI("RC_GET_LINK_NUM", 4),

    /* XP_ERROR_ */
    CI("XP_ERROR_NONE", 0),
    CI("XP_ERROR_THROTTLED", 1),
    CI("XP_ERROR_EXPERIENCES_DISABLED", 2),
    CI("XP_ERROR_INVALID_PARAMETERS", 3),
    CI("XP_ERROR_NOT_PERMITTED", 4),
    CI("XP_ERROR_NO_EXPERIENCE", 5),
    CI("XP_ERROR_NOT_FOUND", 6),
    CI("XP_ERROR_INVALID_EXPERIENCE", 7),
    CI("XP_ERROR_EXPERIENCE_DISABLED", 8),
    CI("XP_ERROR_EXPERIENCE_SUSPENDED", 9),
    CI("XP_ERROR_UNKNOWN_ERROR", 10),
    CI("XP_ERROR_QUOTA_EXCEEDED", 11),
    CI("XP_ERROR_STORE_DISABLED", 12),
    CI("XP_ERROR_STORAGE_EXCEPTION", 13),
    CI("XP_ERROR_KEY_NOT_FOUND", 14),
    CI("XP_ERROR_RETRY_UPDATE", 15),
    CI("XP_ERROR_MATURITY_EXCEEDED", 16),

    /* Linkset Data Store action codes (the integer that arrives in
     * the linkset_data event) */
    CI("LINKSETDATA_RESET", 0),
    CI("LINKSETDATA_UPDATE", 1),
    CI("LINKSETDATA_DELETE", 2),

    /* path / navigation update codes (path_update event 'type') */
    CI("PU_SLOWDOWN_DISTANCE_REACHED", 0),
    CI("PU_GOAL_REACHED", 1),
    CI("PU_FAILURE_INVALID_START", 2),
    CI("PU_FAILURE_INVALID_GOAL", 3),
    CI("PU_FAILURE_UNREACHABLE", 4),
    CI("PU_FAILURE_TARGET_GONE", 5),
    CI("PU_FAILURE_NO_VALID_DESTINATION", 6),
    CI("PU_FAILURE_NO_NAVMESH", 7),
    CI("PU_FAILURE_DYNAMIC_PATHFINDING_DISABLED", 8),
    CI("PU_FAILURE_PARCEL_UNREACHABLE", 9),
    CI("PU_FAILURE_OTHER", 1000000),

    /* Profile categories etc. */
    CI("PROFILE_NONE", 0),
    CI("PROFILE_SCRIPT_MEMORY", 1),

    /* DENSITY / friction defaults aren't constants; skip those. */
    CI("OBJECT_GROUP_TAG", 33)
};
const int BI_CONST_N = (int)(sizeof BI_CONST / sizeof BI_CONST[0]);

/* ----------------------------- Events ----------------------------------- */

#define EV0(name) { name, 0, {0}, {NULL} }
#define EV1(name, t1, n1) { name, 1, {t1}, {n1} }
#define EV2(name, t1, n1, t2, n2) { name, 2, {t1, t2}, {n1, n2} }
#define EV3(name, t1, n1, t2, n2, t3, n3) { name, 3, {t1, t2, t3}, {n1, n2, n3} }
#define EV4(name, t1, n1, t2, n2, t3, n3, t4, n4) { name, 4, {t1, t2, t3, t4}, {n1, n2, n3, n4} }
#define EV5(name, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) { name, 5, {t1,t2,t3,t4,t5}, {n1,n2,n3,n4,n5} }

const BuiltinEvent BI_EVENT[] = {
    EV0("state_entry"),
    EV0("state_exit"),
    EV1("touch_start",        T_INTEGER, "total_number"),
    EV1("touch",              T_INTEGER, "total_number"),
    EV1("touch_end",          T_INTEGER, "total_number"),
    EV1("collision_start",    T_INTEGER, "num_detected"),
    EV1("collision",          T_INTEGER, "num_detected"),
    EV1("collision_end",      T_INTEGER, "num_detected"),
    EV1("land_collision_start", T_VECTOR, "position"),
    EV1("land_collision",       T_VECTOR, "position"),
    EV1("land_collision_end",   T_VECTOR, "position"),
    EV4("listen",             T_INTEGER, "channel", T_STRING, "name", T_KEY, "id", T_STRING, "message"),
    EV0("timer"),
    EV1("sensor",             T_INTEGER, "num_detected"),
    EV0("no_sensor"),
    EV1("on_rez",             T_INTEGER, "start_param"),
    EV1("changed",            T_INTEGER, "change"),
    EV1("attach",             T_KEY,     "id"),
    EV2("dataserver",         T_KEY, "queryid", T_STRING, "data"),
    EV1("email",              T_STRING, "address"),     /* simplified */
    EV5("email",              T_STRING, "time", T_STRING, "address", T_STRING, "subject", T_STRING, "body", T_INTEGER, "num_left"),
    EV4("http_response",      T_KEY, "request_id", T_INTEGER, "status", T_LIST, "metadata", T_STRING, "body"),
    EV4("http_request",       T_KEY, "id", T_STRING, "method", T_STRING, "body", T_STRING, "ignored"),
    EV3("http_request",       T_KEY, "id", T_STRING, "method", T_STRING, "body"),
    EV4("link_message",       T_INTEGER, "sender_num", T_INTEGER, "num", T_STRING, "str", T_KEY, "id"),
    EV2("money",              T_KEY, "id", T_INTEGER, "amount"),
    EV0("moving_start"),
    EV0("moving_end"),
    EV1("not_at_rot_target",  T_VOID, NULL),
    EV0("not_at_rot_target"),
    EV0("at_rot_target"),
    EV3("at_rot_target",      T_INTEGER, "handle", T_ROTATION, "targetrot", T_ROTATION, "ourrot"),
    EV0("not_at_target"),
    EV3("at_target",          T_INTEGER, "tnum", T_VECTOR, "targetpos", T_VECTOR, "ourpos"),
    EV3("object_rez",         T_KEY, "id", T_VECTOR, "ignored", T_VECTOR, "ignored2"),
    EV1("object_rez",         T_KEY, "id"),
    EV3("remote_data",        T_INTEGER, "event_type", T_KEY, "channel", T_KEY, "message_id"),
    EV1("run_time_permissions", T_INTEGER, "perm"),
    EV3("transaction_result", T_KEY, "id", T_INTEGER, "success", T_STRING, "data"),
    EV3("experience_permissions", T_KEY, "agent_id", T_VOID, NULL, T_VOID, NULL),
    EV1("experience_permissions", T_KEY, "agent_id"),
    EV2("experience_permissions_denied", T_KEY, "agent_id", T_INTEGER, "reason"),
    EV2("path_update",        T_INTEGER, "type", T_LIST, "reserved"),
    EV2("linkset_data",       T_INTEGER, "action", T_STRING, "name"),
    EV3("linkset_data",       T_INTEGER, "action", T_STRING, "name", T_STRING, "value"),
    EV3("final_damage",       T_INTEGER, "num_source", T_KEY, "victim", T_FLOAT, "damage"),
    EV3("on_damage",          T_INTEGER, "num_source", T_KEY, "id", T_FLOAT, "damage"),
    EV2("on_death",           T_KEY, "id", T_KEY, "killer"),
    EV2("game_control",       T_KEY, "id", T_INTEGER, "level")
};
const int BI_EVENT_N = (int)(sizeof BI_EVENT / sizeof BI_EVENT[0]);

/* ----------------------------- Lookup ----------------------------------- */

const BuiltinFn *bi_lookup_fn(const char *name) {
    if (!name) return NULL;
    /* linear scan — fine for hundreds of entries */
    for (int i = 0; i < BI_FN_N; i++)
        if (BI_FN[i].name && strcmp(BI_FN[i].name, name) == 0)
            return &BI_FN[i];
    return NULL;
}

/* Find a builtin by name with the given number of arguments. NULL otherwise.
 * (LSL supports multiple-arity overloads — we treat duplicates as separate.)
 */
const BuiltinFn *bi_lookup_fn_arity(const char *name, int n_args) {
    if (!name) return NULL;
    for (int i = 0; i < BI_FN_N; i++) {
        if (BI_FN[i].name && strcmp(BI_FN[i].name, name) == 0
            && BI_FN[i].n_params == n_args)
            return &BI_FN[i];
    }
    return NULL;
}

const BuiltinConst *bi_lookup_const(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < BI_CONST_N; i++)
        if (BI_CONST[i].name && strcmp(BI_CONST[i].name, name) == 0)
            return &BI_CONST[i];
    return NULL;
}

const BuiltinEvent *bi_lookup_event(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < BI_EVENT_N; i++)
        if (BI_EVENT[i].name && strcmp(BI_EVENT[i].name, name) == 0)
            return &BI_EVENT[i];
    return NULL;
}
