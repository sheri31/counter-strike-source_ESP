#pragma once
#include "pch.h"

#define PI (3.14159265)
#define FOV_RANGE_DEVIATION (20)
#define MAX_PLAYER (32)
#define ERROR_OUT_OF_SCREEN (-1000.0f)

typedef struct CSS_Vector {
    float x;
    float y;
    float z;
}CSS_VECTOR, *PCSS_VECTOR;


enum {
    NONE_ROLE,
    OBSERVER,
    TERRORIST,
    COUNTER_TERRORIST
}CAMP_TYPE;

typedef struct CSS_Player {
    BYTE dwUnknown0[(0xE4 - 0)];  
    DWORD dwBlood;   // [[server.dll+4F3FCC] + 0xE4]
    BYTE dwUnknown22[(0x1f4 - 0xE8)];
    DWORD dwCamp;   // [[server.dll+4F3FCC] + 0x1f4]
    BYTE dwUnknown1[(0x280 - 0x1f8)];
    CSS_VECTOR posVec;  // [[server.dll+4F3FCC] + 0x280]
    BYTE dwUnknown2[(0xd08 - 0x28C)];
    DWORD dwArmor;   // [[server.dll+4F3FCC] + 0xD08]
    BYTE dwUnknown3[(0x117C - 0xD0C)];
    DWORD dwMoney;   // [[server.dll+4F3FCC] + 0x117C]
}PLAYER, *PPLAYER;



typedef struct CSS_LocalInfo {
    PLAYER Player[MAX_PLAYER];
    float fFOV;             //[client.dll+5047C0]
    UINT uOtherPlayerCount;      //[server.dll+589868]
    float fMouseAngleY;     //[[engine.dll+0x47F1B4]
    float fMouseAngleX;     //[[engine.dll+0x47F1B8]
}CSS_LOCALINFO, *PCSS_LOCALINFO;



typedef union CSS_FOVRange {
    struct X{
        double lfRightAngle;    //smaller
        double lfLeftAngle;     //bigger
    }X;
    struct Y {
        double lfBottomAngle;    //smaller
        double lfTopAngle;     //bigger
    }Y;
    struct{
        double lfSmallAngle;    //smaller
        double lfBigAngle;     //bigger
    };
}CSS_FOVRANGE, *PCSS_FOVRANGE;

