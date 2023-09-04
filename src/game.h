#ifndef _GAME_H_
#define _GAME_H_

struct GameState
{
    Vec3 playerDir;
    Vec3 playerP;
    Vec3 playerV;
    Vec3 playerA;
    Vec3 playerRot;
    bool playerGrounded;
    bool playerJustJump;

    Vec3 playerLastP;
    Vec3 playerRenderP;

    BSPNode *bspRoot;

    // for rendering
    CBuffer cpuConstBuffer;
    Win32ConstBuffer gpuConstBuffer;
    Win32VertexBuffer vertexBuffer;
};

#endif
