
void GameInit(Memory *memory)
{
    GameState *gameState = (GameState *)memory->data;

    gameState->playerP = {1.2, 1.4, -1.8};
    gameState->playerRot = {0, 0, 0};
    gameState->playerDir = {0, 0, 1};
    gameState->playerGrounded = false;
    gameState->playerJustJump = false;

    gameState->playerLastP = gameState->playerP;
    gameState->playerRenderP = gameState->playerP;
}

void GameUpdate(Memory *memory, f32 dt)
{
    GameState *gameState = (GameState *)memory->data;
    
    if(MouseButtonJustPress(2))
    {
        ShowCursor(false);
    }
    if(MouseButtonJustUp(2))
    {
        ShowCursor(true);
    }
    if(MouseButtonPress(2))
    {
        f32 deltaX = (f32)(MousePosX() - MouseLastPosX()) * 0.0015f;
        f32 deltaY = (f32)(MousePosY() - MouseLastPosY()) * 0.0015f;

        gameState->playerRot.y -= deltaX;
        gameState->playerRot.x -= deltaY;

        if (gameState->playerRot.x > (89.0f/180.0f) * PI)
            gameState->playerRot.x = (89.0f / 180.0f) * PI;
        if (gameState->playerRot.x < -(89.0f / 180.0f) * PI)
            gameState->playerRot.x = -(89.0f/180.0f) * PI;

        SetCursorPos(gWindowX + WINDOW_WIDTH / 2, gWindowY + WINDOW_HEIGHT / 2);
        gCurrentInput.mouseX = WINDOW_WIDTH / 2;
        gCurrentInput.mouseY = WINDOW_HEIGHT / 2;
        gLastInput.mouseX = WINDOW_WIDTH / 2;
        gLastInput.mouseY = WINDOW_HEIGHT / 2;
    }
    
    Vec3 dir = {0, 0, 1};
    gameState->playerDir = Mat4TransformVector(Mat4RotateX(gameState->playerRot.x), dir);
    gameState->playerDir = Mat4TransformVector(Mat4RotateY(gameState->playerRot.y), gameState->playerDir);
    gameState->playerDir = Mat4TransformVector(Mat4RotateZ(gameState->playerRot.z), gameState->playerDir);

    Vec3 playerRight = Vec3Normalized(Vec3Cross(gameState->playerDir, {0, 1, 0}));
    Vec3 forward = Vec3Normalized(Vec3Cross({0, 1, 0}, playerRight));

    Vec3 acc = {};
    if(KeyPress('A'))
    {
        acc = acc + playerRight;
    }
    if(KeyPress('D'))
    {
        acc = acc - playerRight;
    }
    if(KeyPress('W'))
    {
        acc = acc + forward;
    }
    if(KeyPress('S'))
    {
        acc = acc - forward;
    }
    if(KeyJustPress(VK_SPACE) && gameState->playerGrounded)
    {
        gameState->playerJustJump = true;
        Vec3 jumpForce = {0, 6, 0};
        gameState->playerV = gameState->playerV + jumpForce;
    }
    
    acc = (Vec3Normalized(acc) * 12);

    if (!gameState->playerGrounded)
    {
        acc = acc * 0.1f;
    }

    gameState->playerA = acc;

    if (!gameState->playerGrounded)
    {
        Vec3 gravity = {0, -9.8 * 1.5, 0};
        gameState->playerA = gameState->playerA + gravity;
    }

}

void GameFixUpdate(Memory *memory, f32 dt)
{
    GameState *gameState = (GameState *)memory->data;

    gameState->playerLastP = gameState->playerP;

    gameState->playerV = gameState->playerV + gameState->playerA * dt;

    if (gameState->playerGrounded)
    {
        f32 dammping = powf(0.001f, dt);
        gameState->playerV = gameState->playerV * dammping;
    }
    else
    {
        f32 dammping = powf(0.5f, dt);
        gameState->playerV = gameState->playerV * dammping;
    }

    Plane groundPlane = {};
    Vec3 hitP = {};
    Vec3 downRay = {0, -0.1, 0}; // adjust the ray length
    if(RayIntersect(gameState->bspRoot, gameState->playerP, gameState->playerP + downRay, &hitP, &groundPlane))
    {
        Vec3 n = groundPlane.n;
        if(!gameState->playerJustJump)
        {
            gameState->playerV = gameState->playerV - (n * Vec3Dot(gameState->playerV, n));
        }
        gameState->playerGrounded = true;
    }
    else
    {
        gameState->playerGrounded = false;
    }

    if(gameState->playerJustJump && gameState->playerGrounded == false)
    {
        gameState->playerJustJump = false;
    }

    // collision detection and resolution
    Vec3 q;
    Plane plane = {};
    i32 iterations = 100;
    i32 collisionCount = RayIntersect(gameState->bspRoot,
                                      gameState->playerP,
                                      gameState->playerP + gameState->playerV * dt,
                                      &q, &plane);
    while(Vec3LenSq(gameState->playerV) > 0.0f && iterations >= 0)
    {
        if(collisionCount)
        {
            Vec3 n = plane.n;
            Vec3 newP =  q + (n * 0.002f);
            if(!PointInSolidSpace(gameState->bspRoot, newP))
            {
                gameState->playerP = newP;
                gameState->playerV = gameState->playerV - (n * Vec3Dot(gameState->playerV, n));
                collisionCount = RayIntersect(gameState->bspRoot,
                                              gameState->playerP,
                                              gameState->playerP + gameState->playerV * dt,
                                              &q, &plane);
            }
        }
        else
        {
            Vec3 newP = gameState->playerP + gameState->playerV * dt;
            if(!PointInSolidSpace(gameState->bspRoot, newP))
            {
                gameState->playerP = newP;
            }
            break;
        }
        iterations--;
    }
}

void GamePostUpdate(Memory *memory, f32 t)
{
    GameState *gameState = (GameState *)memory->data;
    gameState->playerRenderP = gameState->playerP * t + gameState->playerLastP * (1.0f - t);
}

void GameRender(Memory *memory, Win32Renderer *renderer)
{
    GameState *gameState = (GameState *)memory->data;

    // Update the camera position
    Vec3 up  = {0, 1,  0};
    Vec3 cameraP = gameState->playerRenderP + up * 0.2;
    gameState->cpuConstBuffer.view = Mat4LookAt(cameraP, cameraP + gameState->playerDir, up);
    Win32UpdateConstBuffer(renderer, &gameState->gpuConstBuffer, (void *)&gameState->cpuConstBuffer);

    // Render the game
    u32 stride = sizeof(Vertex);
    u32 offset = 0;
    gameState->cpuConstBuffer.wolrd = Mat4Identity();
    Win32UpdateConstBuffer(renderer, &gameState->gpuConstBuffer, (void *)&gameState->cpuConstBuffer);
    renderer->deviceContext->IASetInputLayout(gameState->vertexBuffer.layout);
    renderer->deviceContext->IASetVertexBuffers(0, 1, &gameState->vertexBuffer.GPUBuffer, &stride, &offset);
    renderer->deviceContext->Draw(gameState->vertexBuffer.verticesCount, 0);
}

void GameShutdown(Memory *memory)
{
    GameState *gameState = (GameState *)memory->data;

    Win32UnloadConstBuffer(&gameState->gpuConstBuffer);
    Win32UnloadVertexBuffer(&gameState->vertexBuffer);
}
