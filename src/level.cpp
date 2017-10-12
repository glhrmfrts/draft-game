// Copyright

template<typename T>
void ReadCollection(memory_arena &Arena, T **Ptr, uint32 *Count, FILE *Handle)
{
    fread(Count, sizeof(uint32), 1, Handle);
    *Ptr = static_cast<T *>(PushSize(Arena, sizeof(T)*(*Count), "level data"));
    fread(*Ptr, sizeof(T), *Count, Handle);
}

template<typename T>
void WriteCollection(T *Ptr, uint32 Count, FILE *Handle)
{
    fwrite(&Count, sizeof(uint32), 1, Handle);
    fwrite(Ptr, sizeof(T), Count, Handle);
}

void ReadLevel(level_data *Data, FILE *Handle)
{
    ReadCollection<char>(Data->TempArena, &Data->Name, &Data->NameLen, Handle);
    ReadCollection<collider_data>(Data->TempArena, &Data->Colliders, &Data->NumColliders, Handle);
    ReadCollection<level_wall_data>(Data->TempArena, &Data->Walls, &Data->NumWalls, Handle);
    ReadCollection<entity_data>(Data->TempArena, &Data->Entities, &Data->NumEntities, Handle);
}

void WriteLevel(level_data *Data, FILE *Handle)
{
    WriteCollection<char>(Data->Name, Data->NameLen, Handle);
    WriteCollection<char>(Data->Colliders, Data->NumColliders, Handle);
    WriteCollection<char>(Data->Walls, Data->NumWalls, Handle);
    WriteCollection<char>(Data->Entities, Data->NumEntities, Handle);
}

void ExtractLevelData(Level *level, LevelData *data)
{
    std::vector<collider *> temp_colliders;
    std::vector<level_wall *> temp_walls;

    data->name_len = strlen(level->name);
    data->name = level->name;
    for (auto *entity : level->entities)
    {
        data->num_entities++;
        if (entity->collider)
        {
            data->num_colliders++;
            temp_colliders.push_back(entity->collider);
        }
        if (entity->wall)
        {
            data->num_walls++;
            temp_walls.push_back(entity->wall);
        }
    }
}
