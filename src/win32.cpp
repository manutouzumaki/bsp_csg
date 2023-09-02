static LRESULT CALLBACK WndProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    switch(Msg)
    {

    case WM_CLOSE:
    {
        gRunnig = false;
    } break;

    default:
    {
        result = DefWindowProcA(hWnd, Msg, wParam, lParam);
    } break;

    }

    return result;
}

static HWND Win32CreateWindow(char *name, i32 x, i32 y, i32 width, i32 height)
{
    HINSTANCE instace = GetModuleHandle(0);
    
    WNDCLASSA wndclass;
    wndclass.style = CS_HREDRAW|CS_VREDRAW;
    wndclass.lpfnWndProc = WndProcA;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = instace;
    wndclass.hIcon = 0;
    wndclass.hCursor = 0;
    wndclass.hbrBackground = 0;
    wndclass.lpszMenuName = 0;
    wndclass.lpszClassName = name;
    RegisterClassA(&wndclass);

    HWND window = CreateWindowExA(
        0, name, name,
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        0, 0, instace, 0);

    return window;
}

static Win32Renderer Win32InitD3D11(HWND window, i32 width, i32 height)
{
    Win32Renderer renderer;
    memset(&renderer, 0, sizeof(Win32Renderer));

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_SOFTWARE
    };
    i32 driverTypesCount = ARRAY_LENGTH(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] = 
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    i32 featureLevelsCount = ARRAY_LENGTH(featureLevels);

    // create the d3d11 device swapchain and device context
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    memset(&swapChainDesc, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = window;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    D3D_DRIVER_TYPE driverType;
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT result = 0;

    for(u32 driver = 0; driver < driverTypesCount; ++driver)
    {
        result = D3D11CreateDeviceAndSwapChain(0, driverTypes[driver], 0, 0, 
                                               featureLevels, featureLevelsCount,
                                               D3D11_SDK_VERSION,
                                               &swapChainDesc, &renderer.swapChain,
                                               &renderer.device, &featureLevel,
                                               &renderer.deviceContext);
        if(SUCCEEDED(result))
        {
            driverType = driverTypes[driver];
            break;
        }
    }
    
    // create render target view
    ID3D11Texture2D *backBufferTexture = 0;
    renderer.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBufferTexture);
    renderer.device->CreateRenderTargetView(backBufferTexture, 0, &renderer.renderTargetView);
    if(backBufferTexture)
    {
        backBufferTexture->Release();
    }

    // set up the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    renderer.deviceContext->RSSetViewports(1, &viewport);

    // create the depth stencil texture
    ID3D11Texture2D* depthStencilTexture = 0;
    D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
    depthStencilTextureDesc.Width = width;
    depthStencilTextureDesc.Height = height;
    depthStencilTextureDesc.MipLevels = 1;
    depthStencilTextureDesc.ArraySize = 1;
    depthStencilTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilTextureDesc.SampleDesc.Count = 1;
    depthStencilTextureDesc.SampleDesc.Quality = 0;
    depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilTextureDesc.CPUAccessFlags = 0;
    depthStencilTextureDesc.MiscFlags = 0;
    // create depth stencil states
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    // Depth test parameters
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    // Stencil test parameters
    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    // Stencil operations if pixel is front-facing
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    // Stencil operations if pixel is back-facing
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    renderer.device->CreateDepthStencilState(&depthStencilDesc, &renderer.depthStencilOn);
    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.StencilEnable = false;
    renderer.device->CreateDepthStencilState(&depthStencilDesc, &renderer.depthStencilOff);

    // create the depth stencil view
    result = renderer.device->CreateTexture2D(&depthStencilTextureDesc, NULL, &depthStencilTexture);
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    result = renderer.device->CreateDepthStencilView(depthStencilTexture, &descDSV, &renderer.depthStencilView);
    if (depthStencilTexture)
    {
        depthStencilTexture->Release();
    }

    // Alpha blending
    D3D11_BLEND_DESC blendStateDesc = {};
    blendStateDesc.RenderTarget[0].BlendEnable = true;
    blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    renderer.device->CreateBlendState(&blendStateDesc, &renderer.alphaBlendEnable);

    blendStateDesc.RenderTarget[0].BlendEnable = false;
    blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    renderer.device->CreateBlendState(&blendStateDesc, &renderer.alphaBlendDisable);

    // Create Rasterizers Types
    D3D11_RASTERIZER_DESC fillRasterizerFrontDesc = {};
    fillRasterizerFrontDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerFrontDesc.CullMode = D3D11_CULL_FRONT;
    fillRasterizerFrontDesc.DepthClipEnable = true;
    renderer.device->CreateRasterizerState(&fillRasterizerFrontDesc, &renderer.fillRasterizerCullFront);

    D3D11_RASTERIZER_DESC fillRasterizerBackDesc = {};
    fillRasterizerBackDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerBackDesc.CullMode = D3D11_CULL_BACK;
    fillRasterizerBackDesc.DepthClipEnable = true;
    renderer.device->CreateRasterizerState(&fillRasterizerBackDesc, &renderer.fillRasterizerCullBack);

    D3D11_RASTERIZER_DESC fillRasterizerNoneDesc = {};
    fillRasterizerNoneDesc.FillMode = D3D11_FILL_SOLID;
    fillRasterizerNoneDesc.CullMode = D3D11_CULL_NONE;
    fillRasterizerNoneDesc.DepthClipEnable = true;
    renderer.device->CreateRasterizerState(&fillRasterizerNoneDesc, &renderer.fillRasterizerCullNone);

    D3D11_RASTERIZER_DESC wireFrameRasterizerDesc = {};
    wireFrameRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireFrameRasterizerDesc.CullMode = D3D11_CULL_NONE;
    wireFrameRasterizerDesc.DepthClipEnable = true;
    renderer.device->CreateRasterizerState(&wireFrameRasterizerDesc, &renderer.wireFrameRasterizer);

    renderer.deviceContext->OMSetRenderTargets(1, &renderer.renderTargetView, renderer.depthStencilView);
    renderer.deviceContext->OMSetDepthStencilState(renderer.depthStencilOn, 1);
    renderer.deviceContext->OMSetBlendState(renderer.alphaBlendEnable, 0, 0xffffffff);
    //renderer.deviceContext->RSSetState(renderer.wireFrameRasterizer);
    renderer.deviceContext->RSSetState(renderer.fillRasterizerCullBack);

    return renderer;
}

void Win32ShutdownD3D11(Win32Renderer *renderer)
{
    if(renderer->device) renderer->device->Release();
    if(renderer->deviceContext) renderer->deviceContext->Release();
    if(renderer->swapChain) renderer->swapChain->Release();
    if(renderer->renderTargetView) renderer->renderTargetView->Release();
    if(renderer->depthStencilView) renderer->depthStencilView->Release();

    if(renderer->wireFrameRasterizer) renderer->wireFrameRasterizer->Release();
    if(renderer->fillRasterizerCullBack) renderer->fillRasterizerCullBack->Release();
    if(renderer->fillRasterizerCullFront) renderer->fillRasterizerCullFront->Release();
    if(renderer->fillRasterizerCullNone) renderer->fillRasterizerCullNone->Release();
    if(renderer->depthStencilOn) renderer->depthStencilOn->Release();
    if(renderer->depthStencilOff) renderer->depthStencilOff->Release();
    if(renderer->alphaBlendEnable) renderer->alphaBlendEnable->Release();
    if(renderer->alphaBlendDisable) renderer->alphaBlendDisable->Release();
}

static void Win32FlushEvents(HWND window)
{
    MSG msg;
    while(PeekMessageA(&msg, window, 0, 0, PM_REMOVE) != 0)
    {
        switch(msg.message)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                bool wasPress = ((msg.lParam & (1 << 30)) != 0);
                bool isPress  = ((msg.lParam & (1 << 31)) == 0);
                if(isPress != wasPress)
                {
                    DWORD vkCode = (DWORD)msg.wParam;
                    gCurrentInput.keys[vkCode].isPress = isPress;
                }
            } break;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            {
                gCurrentInput.mouseButtons[0].isPress = ((msg.wParam & MK_LBUTTON) != 0); 
                gCurrentInput.mouseButtons[1].isPress = ((msg.wParam & MK_MBUTTON) != 0); 
                gCurrentInput.mouseButtons[2].isPress = ((msg.wParam & MK_RBUTTON) != 0); 
            } break;
            case WM_MOUSEMOVE:
            {
                gCurrentInput.mouseX = (i32)GET_X_LPARAM(msg.lParam);
                gCurrentInput.mouseY = (i32)GET_Y_LPARAM(msg.lParam);
            } break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } break;
        }
    }

    for(i32 i = 0; i < ARRAY_LENGTH(gCurrentInput.keys); ++i)
    {
        gCurrentInput.keys[i].wasPress = false;
        if(gLastInput.keys[i].isPress == true)
        {
            gCurrentInput.keys[i].wasPress = true;
        }
    }

    for(i32 i = 0; i < ARRAY_LENGTH(gCurrentInput.mouseButtons); ++i)
    {
        gCurrentInput.mouseButtons[i].wasPress = false;
        if(gLastInput.mouseButtons[i].isPress == true)
        {
            gCurrentInput.mouseButtons[i].wasPress = true;
        }
    }

}

static Win32File Win32ReadFile(char *filepath, Arena *arena)
{
    Win32File result;
    memset(&result, 0, sizeof(Win32File));

    HANDLE hFile = CreateFileA(filepath, GENERIC_READ,
            FILE_SHARE_READ, 0, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, 0);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        printf("Error reading file: %s\n", filepath);
        ASSERT(!"INVALID_CODE_PATH");
    }

    LARGE_INTEGER bytesToRead;
    GetFileSizeEx(hFile, &bytesToRead);

    void *data = ArenaPush(arena, bytesToRead.QuadPart + 1);

    size_t bytesReaded = 0;
    if(!ReadFile(hFile, data, bytesToRead.QuadPart, (LPDWORD)&bytesReaded, 0))
    {
        printf("Error reading file: %s\n", filepath);
        ASSERT(!"INVALID_CODE_PATH");
    }

    char *end = ((char *)data) + bytesToRead.QuadPart;
    end[0] = '\0';

    CloseHandle(hFile);

    result.data = data;
    result.size = bytesReaded;

    return result;
}

static Win32Shader Win32LoadShader(Win32Renderer *renderer,
                                   Memory *memory, char *vertpath, char *fragpath)
{
    Win32Shader shader = {};

    Arena tmpArena = ArenaTmpBegin(memory, KILOBYTES(1));

    Win32File vertfile = Win32ReadFile(vertpath, &tmpArena);
    Win32File fragfile = Win32ReadFile(fragpath, &tmpArena);
    
    HRESULT result = 0;
    ID3DBlob *errorVertexShader = 0;
    result = D3DCompile(vertfile.data, vertfile.size,
                        0, 0, 0, "vs_main", "vs_4_0",
                        D3DCOMPILE_ENABLE_STRICTNESS, 0,
                        &shader.vertexShaderCompiled,
                        &errorVertexShader);
    if(errorVertexShader != 0)
    {
        char *errorString = (char *)errorVertexShader->GetBufferPointer();
        printf("error compiling vertex shader (%s): %s", vertpath, errorString);
        errorVertexShader->Release();
        ASSERT(!"INVALID_CODE_PATH");
    }

    ID3DBlob *errorFragmentShader = 0;
    result = D3DCompile(fragfile.data, fragfile.size,
                        0, 0, 0, "fs_main", "ps_4_0",
                        D3DCOMPILE_ENABLE_STRICTNESS, 0,
                        &shader.fragmentShaderCompiled,
                        &errorFragmentShader);
    if(errorFragmentShader)
    {
        char *errorString = (char *)errorFragmentShader->GetBufferPointer();
        printf("error compiling fragment shader (%s): %s", fragpath, errorString);
        errorFragmentShader->Release();
        ASSERT(!"INVALID_CODE_PATH")
    }

    // create the vertex and fragment shader
    result = renderer->device->CreateVertexShader(
            shader.vertexShaderCompiled->GetBufferPointer(),
            shader.vertexShaderCompiled->GetBufferSize(), 0,
            &shader.vertex);
    result = renderer->device->CreatePixelShader(
            shader.fragmentShaderCompiled->GetBufferPointer(),
            shader.fragmentShaderCompiled->GetBufferSize(), 0,
            &shader.fragment);

    ArenaTmpEnd(memory, &tmpArena);

    return shader;
}

void Win32UnloadShader(Win32Shader *shader)
{
    if(shader->vertex) shader->vertex->Release();
    if(shader->fragment) shader->fragment->Release();
    if(shader->vertexShaderCompiled) shader->vertexShaderCompiled->Release();
    if(shader->fragmentShaderCompiled) shader->fragmentShaderCompiled->Release();
}

Win32ConstBuffer Win32LoadConstBuffer(Win32Renderer *renderer,
                                      void *bufferData, size_t bufferSize, u32 index)
{
    Win32ConstBuffer constBuffer = {};

    D3D11_BUFFER_DESC constBufferDesc;
    ZeroMemory(&constBufferDesc, sizeof(constBufferDesc));
    constBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constBufferDesc.ByteWidth = bufferSize;
    constBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    HRESULT result = renderer->device->CreateBuffer(&constBufferDesc, 0, &constBuffer.buffer);
    if(FAILED(result))
    {
        printf("error creating const buffer\n");
        ASSERT(!"INVALID_CODE_PATH");
    }
    renderer->deviceContext->UpdateSubresource(constBuffer.buffer, 0, 0, bufferData, 0, 0);
    renderer->deviceContext->VSSetConstantBuffers(index, 1, &constBuffer.buffer);
    renderer->deviceContext->PSSetConstantBuffers(index, 1, &constBuffer.buffer);
    constBuffer.index = index;

    return constBuffer;
}

void Win32UpdateConstBuffer(Win32Renderer *renderer,
                            Win32ConstBuffer *constBuffer, void *bufferData)
{
    renderer->deviceContext->UpdateSubresource(constBuffer->buffer, 0, 0, bufferData, 0, 0);
    renderer->deviceContext->VSSetConstantBuffers(constBuffer->index, 1, &constBuffer->buffer);
    renderer->deviceContext->PSSetConstantBuffers(constBuffer->index, 1, &constBuffer->buffer);
}

void Win32UnloadConstBuffer(Win32ConstBuffer *constBuffer)
{
    if(constBuffer->buffer) constBuffer->buffer->Release();
    constBuffer->index = -1;
}

Win32VertexBuffer Win32LoadVertexBuffer(Win32Renderer *renderer,
                                        Vertex *vertices, size_t count, ID3D11InputLayout *layout)
{
    Win32VertexBuffer buffer = {};
    buffer.verticesCount = count;
    buffer.layout = layout;

    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));

    D3D11_BUFFER_DESC vertexDesc;
    ZeroMemory(&vertexDesc, sizeof(vertexDesc));
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(Vertex) * count;
    resourceData.pSysMem = vertices;

    HRESULT result = renderer->device->CreateBuffer(&vertexDesc, &resourceData, &buffer.GPUBuffer);
    if(FAILED(result))
    {
        printf("error loading vertex buffer\n");
        ASSERT(!"INVALID_CODE_PATH");
    }

    return buffer;
}

void Win32UnloadVertexBuffer(Win32VertexBuffer *vertexBuffer)
{
    if(vertexBuffer->GPUBuffer) vertexBuffer->GPUBuffer->Release();
    vertexBuffer->verticesCount = 0;
}


