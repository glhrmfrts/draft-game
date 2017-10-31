// Copyright

#define ROAD_LANE_WIDTH   2
#define LEVEL_PLANE_COUNT 5
#define SHIP_Z            0.2f

static void UpdateCameraToPlayer(camera &cam, entity *playerEntity, float dt)
{
    const float CAMERA_LERP = 10.0f;
    vec3 d = playerEntity->Pos() - cam.LookAt;
    cam.Position += d * CAMERA_LERP * dt;
    cam.LookAt = cam.Position + vec3{0, -Global_Camera_OffsetY, -Global_Camera_OffsetZ};
}

static void InitMenu(game_main *g)
{
    g->State = GameState_Menu;
    FreeArena(g->World.Arena);

    g->World.Camera = &g->Camera;
    g->PlayerEntity = CreateShipEntity(&g->World.Arena, GetShipMesh(g), Color_blue, IntColor(FirstPalette.Colors[1]), true);
    g->PlayerEntity->Transform.Position.z = SHIP_Z;
    g->PlayerEntity->Transform.Velocity.y = PLAYER_MIN_VEL;
    AddEntity(g->World, g->PlayerEntity);

    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g->World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = -0.25f;
        ent->Transform.Scale = vec3{LEVEL_PLANE_SIZE, LEVEL_PLANE_SIZE, 0};
        ent->Model = CreateModel(&g->World.Arena, GetFloorMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g->World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE/2;
        AddEntity(g->World, ent);
    }
    for (int i = 0; i < LEVEL_PLANE_COUNT; i++)
    {
        auto ent = PushStruct<entity>(g->World.Arena);
        ent->Transform.Position.y = LEVEL_PLANE_SIZE * i;
        ent->Transform.Position.z = 0.0f;
        ent->Transform.Scale = vec3{ 2, LEVEL_PLANE_SIZE, 1 };
        ent->Model = CreateModel(&g->World.Arena, GetRoadMesh(g));
        ent->Repeat = PushStruct<entity_repeat>(g->World.Arena);
        ent->Repeat->Count = LEVEL_PLANE_COUNT;
        ent->Repeat->Size = LEVEL_PLANE_SIZE;
        ent->Repeat->DistanceFromCamera = LEVEL_PLANE_SIZE / 2;
        AddEntity(g->World, ent);
    }

    g->TestFont = FindBitmapFont(g->AssetLoader, "g_type_16");
}

static void RenderMenu(game_main *g, float dt)
{
    static color textColor = Color_white;
    static color textSelectedColor = IntColor(ShipPalette.Colors[SHIP_ORANGE]);

    g->PlayerEntity->Vel().y = PLAYER_MIN_VEL;
    g->PlayerEntity->Pos().y += g->PlayerEntity->Vel().y * dt;

    UpdateLogiclessEntities(g->World, dt);
    UpdateCameraToPlayer(g->Camera, g->PlayerEntity, dt);
    UpdateProjectionView(g->Camera);
    PostProcessBegin(g->RenderState);
    RenderBegin(g->RenderState, dt);
    RenderEntityWorld(g->RenderState, g->World, dt);
    RenderEnd(g->RenderState, g->Camera);
    PostProcessEnd(g->RenderState);

    UpdateProjectionView(g->GUICamera);
    Begin(g->GUI, g->GUICamera);
    DrawTextCentered(g->GUI, g->TestFont, "PLAY", rect{146,16,0,0}, textSelectedColor);
    DrawTextCentered(g->GUI, g->TestFont, "OPTS", rect{232,16,0,0}, textColor);
    DrawTextCentered(g->GUI, g->TestFont, "CRED", rect{408,16,0,0}, textColor);
    DrawTextCentered(g->GUI, g->TestFont, "QUIT", rect{496,16,0,0}, textColor);
    End(g->GUI);
}
