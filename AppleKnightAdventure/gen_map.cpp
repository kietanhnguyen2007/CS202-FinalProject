#include <iostream>
#include <fstream>
#include <string>
#include <vector>

const int WIDTH_TILES = 1000;
const int HEIGHT_TILES = 500;
const int TILE_SIZE = 64;

int main() {
    std::ofstream out("assets/levels/world.ldtk");
    if (!out.is_open()) {
        std::cerr << "Failed to open assets/levels/world.ldtk for writing\n";
        return 1;
    }

    std::vector<int> intGrid(WIDTH_TILES * HEIGHT_TILES, 0);
    int floor_start_y = HEIGHT_TILES - 5;
    
    std::string gridTiles = "[\n";
    bool first_tile = true;

    for (int y = 0; y < HEIGHT_TILES; ++y) {
        for (int x = 0; x < WIDTH_TILES; ++x) {
            int idx = y * WIDTH_TILES + x;
            bool is_wall = (x < 2 || x > WIDTH_TILES - 3);
            bool is_floor = (y >= floor_start_y);
            
            if (is_wall || is_floor) {
                intGrid[idx] = 1;
                int tile_id = (y > floor_start_y) ? 26 : 1;
                
                if (!first_tile) gridTiles += ",\n";
                gridTiles += "{\"px\":[" + std::to_string(x * TILE_SIZE) + "," + std::to_string(y * TILE_SIZE) + "],\"t\":" + std::to_string(tile_id) + ",\"f\":0}";
                first_tile = false;
            }
        }
    }

    for (int x = 10; x < WIDTH_TILES - 10; x += 20) {
        int plat_y = floor_start_y - 4;
        for (int px = x; px < x + 5; ++px) {
            int idx = plat_y * WIDTH_TILES + px;
            intGrid[idx] = 1;
            
            if (!first_tile) gridTiles += ",\n";
            gridTiles += "{\"px\":[" + std::to_string(px * TILE_SIZE) + "," + std::to_string(plat_y * TILE_SIZE) + "],\"t\":1,\"f\":0}";
            first_tile = false;
        }
    }
    gridTiles += "\n]";

    std::string intGridCsv = "[\n";
    for (size_t i = 0; i < intGrid.size(); ++i) {
        intGridCsv += std::to_string(intGrid[i]);
        if (i < intGrid.size() - 1) intGridCsv += ",";
    }
    intGridCsv += "\n]";

    int spawnX = 4 * TILE_SIZE;
    int spawnY = (floor_start_y - 2) * TILE_SIZE;
    int endX = (WIDTH_TILES - 10) * TILE_SIZE;
    int endY = (floor_start_y - 2) * TILE_SIZE;

    std::string json = R"({
  "__header__": {
    "fileType": "LDtk Project JSON",
    "app": "LDtk",
    "appVersion": "1.5.3"
  },
  "jsonVersion": "1.5.3",
  "defaultGridSize": 64,
  "bgColor": "#2B2B2B",
  "defs": {
    "layers": [
      { "__type": "IntGrid", "identifier": "Collision", "type": "IntGrid", "uid": 1, "gridSize": 64, "intGridValues": [{"value": 1, "identifier": "Solid"}] },
      { "__type": "Tiles", "identifier": "Tiles", "type": "Tiles", "uid": 2, "gridSize": 64, "tilesetDefUid": 1 },
      { "__type": "Tiles", "identifier": "BG_Tiles", "type": "Tiles", "uid": 3, "gridSize": 64, "tilesetDefUid": 1 },
      { "__type": "Entities", "identifier": "Entities", "type": "Entities", "uid": 4, "gridSize": 64 }
    ],
    "entities": [
      { "identifier": "SpawnSolo", "uid": 100, "width": 64, "height": 64 },
      { "identifier": "CheckpointEnd", "uid": 101, "width": 64, "height": 64 }
    ],
    "tilesets": [
      { "identifier": "Tiles", "uid": 1, "relPath": "assets/textures/tiles/Tiles.png", "pxWid": 1600, "pxHei": 1600, "tileGridSize": 64, "__cWid": 25, "__cHei": 25 }
    ]
  },
  "levels": [
    {
      "identifier": "Level_0",
      "uid": 1,
      "pxWid": )" + std::to_string(WIDTH_TILES * TILE_SIZE) + R"(,
      "pxHei": )" + std::to_string(HEIGHT_TILES * TILE_SIZE) + R"(,
      "layerInstances": [
        {
          "__identifier": "Entities",
          "__type": "Entities",
          "layerDefUid": 4,
          "entityInstances": [
            { "__identifier": "SpawnSolo", "defUid": 100, "px": [)" + std::to_string(spawnX) + "," + std::to_string(spawnY) + R"(], "fieldInstances": [] },
            { "__identifier": "CheckpointEnd", "defUid": 101, "px": [)" + std::to_string(endX) + "," + std::to_string(endY) + R"(], "fieldInstances": [] }
          ]
        },
        {
          "__identifier": "BG_Tiles",
          "__type": "Tiles",
          "layerDefUid": 3,
          "__tilesetDefUid": 1,
          "gridTiles": [],
          "entityInstances": []
        },
        {
          "__identifier": "Tiles",
          "__type": "Tiles",
          "layerDefUid": 2,
          "__tilesetDefUid": 1,
          "gridTiles": )" + gridTiles + R"(,
          "entityInstances": []
        },
        {
          "__identifier": "Collision",
          "__type": "IntGrid",
          "layerDefUid": 1,
          "intGridCsv": )" + intGridCsv + R"(,
          "gridTiles": [],
          "entityInstances": []
        }
      ]
    }
  ]
})";

    out << json;
    out.close();
    std::cout << "Generated 1000x500 map at assets/levels/world.ldtk\n";
    return 0;
}
