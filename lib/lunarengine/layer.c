#include "linked_list.h"
#include "lunarengine.h"

#include <stdlib.h>

typedef struct {
    float scrollOffsetX, scrollSpeedX;
    float scrollOffsetY, scrollSpeedY;
    float scaleW, scaleH;
    enum LE_LayerType type;
    void* ptr;
    LE_LayerList* parent;
    struct {
        float camPosX;
        float camPosY;
    } cameraData;
} _LE_Layer;

typedef DEFINE_LIST(_LE_Layer) _LE_LayerList;

LE_LayerList* LE_CreateLayerList() {
    struct LinkedList__LE_Layer* list = LE_LL_Create();
    list->value = malloc(sizeof(_LE_Layer));
    list->value->cameraData.camPosX = 0;
    list->value->cameraData.camPosY = 0;
    return (LE_LayerList*)list;
}

LE_Layer* LE_MakeLayer(LE_LayerList* layers, void* data, enum LE_LayerType type) {
    _LE_Layer* l = malloc(sizeof(_LE_Layer));
    l->scrollOffsetX = l->scrollOffsetY = 0;
    l->scrollSpeedX = l->scrollSpeedY = 1;
    l->scaleW = l->scaleH = 1;
    l->type = type;
    l->ptr = data;
    l->parent = (LE_LayerList*)LE_LL_Add(layers, l);
    return (LE_Layer*)l;
}

LE_Layer* LE_AddTilemapLayer(LE_LayerList* layers, LE_Tilemap* tilemap) {
    return LE_MakeLayer(layers, tilemap, LE_LayerType_Tilemap);
}

LE_Layer* LE_AddEntityLayer(LE_LayerList* layers, LE_EntityList* entities) {
    return LE_MakeLayer(layers, entities, LE_LayerType_Entity);
}

LE_Layer* LE_AddCustomLayer(LE_LayerList* layers, CustomLayer callback) {
    return LE_MakeLayer(layers, callback, LE_LayerType_Custom);
}

void LE_MoveLayer(LE_Layer* layer, int index) {
    _LE_Layer* l = (_LE_Layer*)layer;
    _LE_LayerList* ll = (_LE_LayerList*)l->parent;
    if (ll->prev) ll->prev->next = ll->next;
    if (ll->next) ll->next->prev = ll->prev;
    _LE_LayerList* prev = ll->frst;
    for (int i = 0; i < index; i++) {
        if (!prev->next) break;
    }
    _LE_LayerList* next = prev->next;
    prev->next = ll;
    if (next) next->prev = ll;
    ll->prev = prev;
    ll->next = next;
}

int LE_IndexOfLayer(LE_Layer* layer) {
    _LE_Layer* l = (_LE_Layer*)layer;
    _LE_LayerList* ll = (_LE_LayerList*)l->parent;
    int counter = 0;
    while (ll->prev->prev) {
        counter++;
        ll = ll->prev;
    }
    return counter;
}

void LE_ScrollCamera(LE_LayerList* layers, float camX, float camY) {
    _LE_LayerList* ll = (_LE_LayerList*)layers;
    ll->value->cameraData.camPosX = camX;
    ll->value->cameraData.camPosY = camY;
}

void LE_GetCameraPos(LE_LayerList* layers, float* camX, float* camY) {
    _LE_LayerList* ll = (_LE_LayerList*)layers;
    if (camX) *camX = ll->value->cameraData.camPosX;
    if (camY) *camY = ll->value->cameraData.camPosY;
}

enum LE_LayerType LE_LayerGetType(LE_Layer* layer) {
    return ((_LE_Layer*)layer)->type;
}

void* LE_LayerGetDataPointer(LE_Layer* layer) {
    return ((_LE_Layer*)layer)->ptr;
}

void LE_Draw(LE_LayerList* layers, int screenW, int screenH, LE_DrawList* dl) {
    _LE_LayerList* ll = (_LE_LayerList*)layers;
    while (ll->next) {
        ll = ll->next;
        LE_DrawSingleLayer((LE_Layer*)ll->value, screenW, screenH, dl);
    }
}

void LE_DrawSingleLayer(LE_Layer* layer, int screenW, int screenH, LE_DrawList* dl) {
    _LE_Layer* l = (_LE_Layer*)layer;
    _LE_LayerList* ll = ((_LE_LayerList*)l->parent)->frst;
    l->scrollOffsetX = ll->value->cameraData.camPosX * l->scrollSpeedX;
    l->scrollOffsetY = ll->value->cameraData.camPosY * l->scrollSpeedY;
    float tlx = screenW / -2.f / l->scaleW + l->scrollOffsetX;
    float tly = screenH / -2.f / l->scaleH + l->scrollOffsetY;
    float brx = screenW /  2.f / l->scaleW + l->scrollOffsetX;
    float bry = screenH /  2.f / l->scaleH + l->scrollOffsetY;
    switch (l->type) {
        case LE_LayerType_Tilemap:
            {
                LE_Tilemap* tilemap = l->ptr;
                LE_Tileset* tileset = LE_TilemapGetTileset(tilemap);
                if (!tileset) return;
                int w, h;
                LE_TilesetGetTileSize(tileset, &w, &h);
                LE_DrawPartialTilemap(l->ptr, -l->scrollOffsetX * l->scaleW + screenW / 2.f, -l->scrollOffsetY * l->scaleH + screenH / 2.f, tlx - 1, tly - 1, brx + 1, bry + 1, l->scaleW / w, l->scaleH / h, dl);
            }
            break;
        case LE_LayerType_Entity:
            {
                LE_EntityListIter* iter = LE_EntityListGetIter(l->ptr);
                while (iter) {
                    LE_Entity* entity = LE_EntityListGet(iter);
                    LE_DrawEntity(entity, (entity->posX - l->scrollOffsetX) * l->scaleW + screenW / 2.f, (entity->posY - l->scrollOffsetY) * l->scaleH + screenH / 2.f, 1, 1, dl);
                    iter = LE_EntityListNext(iter);
                }
            }
            break;
        case LE_LayerType_Custom:
            ((CustomLayer)l->ptr)(l->scrollOffsetX, l->scrollOffsetY, l->scaleW, l->scaleH);
            break;
    }
}

void LE_DestroyLayer(LE_Layer* layer) {
    _LE_LayerList* ll = ((_LE_LayerList*)((_LE_Layer*)layer)->parent)->frst;
    LE_LL_Remove(ll, layer);
    free(layer);
}

void LE_DestroyLayerList(LE_LayerList* layers) {
    LE_LL_DeepFree(layers, free);
}

int LE_NumLayers(LE_LayerList* layers) {
    return LE_LL_Size(layers);
}

LE_LayerListIter* LE_LayerListGetIter(LE_LayerList* list) {
    return LE_LayerListNext((LE_LayerListIter*)list);
}

LE_LayerListIter* LE_LayerListNext(LE_LayerListIter* iter) {
    return (LE_LayerListIter*)((_LE_LayerList*)iter)->next;
}

LE_LayerListIter* LE_LayerListPrev(LE_LayerListIter* iter) {
    _LE_LayerList* ll = (_LE_LayerList*)iter;
    if (!ll->prev->prev) return NULL;
    return (LE_LayerListIter*)ll->prev;
}

LE_Layer* LE_LayerListGet(LE_LayerListIter* iter) {
    return (LE_Layer*)((_LE_LayerList*)iter)->value;
}