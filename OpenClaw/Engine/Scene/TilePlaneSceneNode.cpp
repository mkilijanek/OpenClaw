#include <SDL2/SDL.h>
#include <algorithm>
#include "Scene.h"
#include "TilePlaneSceneNode.h"
#include "../Actor/Components/RenderComponent.h"
#include "../Graphics2D/Image.h"
#include "../GameApp/BaseGameApp.h"

SDL2TilePlaneSceneNode::SDL2TilePlaneSceneNode(const uint32 actorId,
    BaseRenderComponent* pRenderComponent,
    RenderPass renderPass,
    Point position)
    : SceneNode(actorId, pRenderComponent, renderPass, position)
{

}

SDL2TilePlaneSceneNode::~SDL2TilePlaneSceneNode()
{

}

void SDL2TilePlaneSceneNode::VRender(Scene* pScene)
{
    TilePlaneRenderComponent* pRenderComponent = static_cast<TilePlaneRenderComponent*>(m_pRenderComponent);

    const TilePlaneProperties* pProperties = pRenderComponent->GetTilePlaneProperties();
    const TileImageList* pImageList = pRenderComponent->GetTileImageList();

    shared_ptr<CameraNode> camera = pScene->GetCamera();
    SDL_Renderer* renderer = pScene->GetRenderer();

    // Multiple times user variables
    int32 tilePixelWidth = pProperties->tilePixelWidth;
    int32 tilePixelHeight = pProperties->tilePixelHeight;

    const SDL_Rect cameraRect = camera->GetCameraRect();

    float movementRatioX = pProperties->movementPercentX / 100.0f;
    float movementRatioY = pProperties->movementPercentY / 100.0f;

    float parallaxCameraPosX = (float) cameraRect.x * movementRatioX;
    float parallaxCameraPosY = (float) cameraRect.y * movementRatioY;

    const auto wrapIndex = [](int32 value, int32 modulo) -> int32 {
        int32 wrapped = value % modulo;
        return wrapped < 0 ? wrapped + modulo : wrapped;
    };

    int32_t startCol = (int32_t)(parallaxCameraPosX / tilePixelWidth);
    int32_t startRow = (int32_t)(parallaxCameraPosY / tilePixelHeight);

    int32_t colTilesToRender = (cameraRect.w + tilePixelWidth - 1) / tilePixelWidth + 1;
    int32_t rowTilesToRender = (cameraRect.h + tilePixelHeight - 1) / tilePixelHeight + 1;

    if (!pProperties->isWrappedX)
    {
        int32_t endCol = std::min<int32_t>(startCol + colTilesToRender, pProperties->tilesOnAxisX);
        startCol = std::max<int32_t>(startCol, 0);
        colTilesToRender = std::max<int32_t>(endCol - startCol, 0);
    }

    if (!pProperties->isWrappedY)
    {
        int32_t endRow = std::min<int32_t>(startRow + rowTilesToRender, pProperties->tilesOnAxisY);
        startRow = std::max<int32_t>(startRow, 0);
        rowTilesToRender = std::max<int32_t>(endRow - startRow, 0);
    }

    for (int32_t rowOffset = 0; rowOffset < rowTilesToRender; ++rowOffset)
    {
        int32_t row = startRow + rowOffset;
        const int rowTileIndex = pProperties->isWrappedY
            ? wrapIndex(row, pProperties->tilesOnAxisY)
            : row;

        for (int32_t colOffset = 0; colOffset < colTilesToRender; ++colOffset)
        {
            int32_t col = startCol + colOffset;
            const int colTileIndex = pProperties->isWrappedX
                ? wrapIndex(col, pProperties->tilesOnAxisX)
                : col;

            Image* image = (*pImageList)[rowTileIndex * pProperties->tilesOnAxisX + colTileIndex];

            if (image && image->GetTexture() != NULL)
            {
                int32_t x = col * tilePixelWidth - parallaxCameraPosX;
                int32_t y = row * tilePixelHeight - parallaxCameraPosY;
                SDL_Rect tileRect = { x,
                    y,
                    tilePixelWidth,
                    tilePixelHeight };

                SDL_RenderCopy(renderer, image->GetTexture(), NULL, &tileRect);
            }
        }
    }
}
