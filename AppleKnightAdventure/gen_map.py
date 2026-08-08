import json
import os

WIDTH_TILES = 1000
HEIGHT_TILES = 500
TILE_SIZE = 64

def generate_ldtk():
    # IntGrid CSV: 0 for empty, 1 for solid
    # Let's make a floor at the bottom 5 tiles
    # and some walls at the edges
    intGridCsv = [0] * (WIDTH_TILES * HEIGHT_TILES)
    gridTiles = []
    
    # Fill floor
    floor_start_y = HEIGHT_TILES - 5
    for y in range(HEIGHT_TILES):
        for x in range(WIDTH_TILES):
            idx = y * WIDTH_TILES + x
            
            # Left/Right walls
            is_wall = (x < 2 or x > WIDTH_TILES - 3)
            # Floor
            is_floor = (y >= floor_start_y)
            
            if is_wall or is_floor:
                intGridCsv[idx] = 1
                
                # Assign a basic tile
                # Assuming Tiles.png has 25 columns. 
                # Top grass: id 1 (just guessing), Dirt: id 26
                tile_id = 26 if y > floor_start_y else 1
                gridTiles.append({
                    "px": [x * TILE_SIZE, y * TILE_SIZE],
                    "t": tile_id,
                    "f": 0
                })
                
    # Add some random platforms so it's not completely flat
    # every 20 tiles horizontally, put a platform
    for x in range(10, WIDTH_TILES - 10, 20):
        plat_y = floor_start_y - 4
        for px in range(x, x + 5):
            intGridCsv[plat_y * WIDTH_TILES + px] = 1
            gridTiles.append({
                "px": [px * TILE_SIZE, plat_y * TILE_SIZE],
                "t": 1,
                "f": 0
            })

    # Entities
    entities = [
        {
            "__identifier": "SpawnSolo",
            "__grid": [4, floor_start_y - 2],
            "__pivot": [0, 0],
            "iid": "iid_spawn",
            "width": 64,
            "height": 64,
            "defUid": 100,
            "px": [4 * TILE_SIZE, (floor_start_y - 2) * TILE_SIZE],
            "fieldInstances": []
        },
        {
            "__identifier": "CheckpointEnd",
            "__grid": [WIDTH_TILES - 10, floor_start_y - 2],
            "__pivot": [0, 0],
            "iid": "iid_end",
            "width": 64,
            "height": 64,
            "defUid": 101,
            "px": [(WIDTH_TILES - 10) * TILE_SIZE, (floor_start_y - 2) * TILE_SIZE],
            "fieldInstances": []
        }
    ]

    layerInstances = [
        {
            "__identifier": "Entities",
            "__type": "Entities",
            "__cWid": WIDTH_TILES,
            "__cHei": HEIGHT_TILES,
            "__gridSize": TILE_SIZE,
            "__opacity": 1,
            "__pxTotalOffsetX": 0,
            "__pxTotalOffsetY": 0,
            "__tilesetDefUid": None,
            "__tilesetRelPath": None,
            "levelId": 1,
            "layerDefUid": 4,
            "pxOffsetX": 0,
            "pxOffsetY": 0,
            "visible": True,
            "optionalRules": [],
            "intGridCsv": [],
            "autoLayerTiles": [],
            "seed": 1,
            "overrideTilesetUid": None,
            "gridTiles": [],
            "entityInstances": entities
        },
        {
            "__identifier": "BG_Tiles",
            "__type": "Tiles",
            "__cWid": WIDTH_TILES,
            "__cHei": HEIGHT_TILES,
            "__gridSize": TILE_SIZE,
            "__opacity": 1,
            "__pxTotalOffsetX": 0,
            "__pxTotalOffsetY": 0,
            "__tilesetDefUid": 1,
            "__tilesetRelPath": "assets/textures/tiles/Tiles.png",
            "levelId": 1,
            "layerDefUid": 3,
            "pxOffsetX": 0,
            "pxOffsetY": 0,
            "visible": True,
            "optionalRules": [],
            "intGridCsv": [],
            "autoLayerTiles": [],
            "seed": 1,
            "overrideTilesetUid": None,
            "gridTiles": [],
            "entityInstances": []
        },
        {
            "__identifier": "Tiles",
            "__type": "Tiles",
            "__cWid": WIDTH_TILES,
            "__cHei": HEIGHT_TILES,
            "__gridSize": TILE_SIZE,
            "__opacity": 1,
            "__pxTotalOffsetX": 0,
            "__pxTotalOffsetY": 0,
            "__tilesetDefUid": 1,
            "__tilesetRelPath": "assets/textures/tiles/Tiles.png",
            "levelId": 1,
            "layerDefUid": 2,
            "pxOffsetX": 0,
            "pxOffsetY": 0,
            "visible": True,
            "optionalRules": [],
            "intGridCsv": [],
            "autoLayerTiles": [],
            "seed": 1,
            "overrideTilesetUid": None,
            "gridTiles": gridTiles,
            "entityInstances": []
        },
        {
            "__identifier": "Collision",
            "__type": "IntGrid",
            "__cWid": WIDTH_TILES,
            "__cHei": HEIGHT_TILES,
            "__gridSize": TILE_SIZE,
            "__opacity": 1,
            "__pxTotalOffsetX": 0,
            "__pxTotalOffsetY": 0,
            "__tilesetDefUid": None,
            "__tilesetRelPath": None,
            "levelId": 1,
            "layerDefUid": 1,
            "pxOffsetX": 0,
            "pxOffsetY": 0,
            "visible": True,
            "optionalRules": [],
            "intGridCsv": intGridCsv,
            "autoLayerTiles": [],
            "seed": 1,
            "overrideTilesetUid": None,
            "gridTiles": [],
            "entityInstances": []
        }
    ]

    level = {
        "identifier": "Level_0",
        "iid": "level_0_iid",
        "uid": 1,
        "worldX": 0,
        "worldY": 0,
        "worldDepth": 0,
        "pxWid": WIDTH_TILES * TILE_SIZE,
        "pxHei": HEIGHT_TILES * TILE_SIZE,
        "__bgColor": "#2B2B2B",
        "bgColor": None,
        "useAutoIdentifier": False,
        "bgRelPath": None,
        "bgPos": None,
        "bgPivotX": 0.5,
        "bgPivotY": 0.5,
        "__smartColor": "#2B2B2B",
        "__bgPos": None,
        "externalRelPath": None,
        "fieldInstances": [],
        "layerInstances": layerInstances,
        "__neighbours": []
    }

    ldtk_data = {
        "__header__": {
            "fileType": "LDtk Project JSON",
            "app": "LDtk",
            "doc": "https://ldtk.io/json",
            "schema": "https://ldtk.io/files/JSON_SCHEMA.json",
            "appAuthor": "Sebastien 'deepnight' Benard",
            "appVersion": "1.5.3",
            "url": "https://ldtk.io"
        },
        "jsonVersion": "1.5.3",
        "appBuildId": 473703,
        "nextUid": 200,
        "identifierStyle": "Capitalize",
        "toc": [],
        "worldLayout": "Free",
        "worldGridWidth": 64,
        "worldGridHeight": 64,
        "defaultLevelWidth": 640,
        "defaultLevelHeight": 640,
        "defaultPivotX": 0,
        "defaultPivotY": 0,
        "defaultGridSize": 64,
        "defaultEntityWidth": 64,
        "defaultEntityHeight": 64,
        "bgColor": "#2B2B2B",
        "defaultLevelBgColor": "#2B2B2B",
        "minifyJson": False,
        "externalLevels": False,
        "exportTiled": False,
        "simplifiedExport": False,
        "imageExportMode": "None",
        "exportLevelBg": True,
        "pngFilePattern": None,
        "backupOnSave": False,
        "backupLimit": 10,
        "backupRelPath": None,
        "levelNamePattern": "Level_%idx",
        "tutorialDesc": None,
        "customCommands": [],
        "flags": ["PrependIndexToLevelFileNames"],
        "defs": {
            "layers": [
                {
                    "__type": "IntGrid",
                    "identifier": "Collision",
                    "type": "IntGrid",
                    "uid": 1,
                    "gridSize": 64,
                    "guideGridWid": 0,
                    "guideGridHei": 0,
                    "displayOpacity": 1,
                    "inactiveOpacity": 0.5,
                    "hideInList": False,
                    "hideFieldsWhenInactive": False,
                    "canSelectWhenInactive": True,
                    "renderInWorldView": True,
                    "pxOffsetX": 0,
                    "pxOffsetY": 0,
                    "parallaxFactorX": 0,
                    "parallaxFactorY": 0,
                    "parallaxScaling": True,
                    "requiredTags": [],
                    "excludedTags": [],
                    "autoTilesetDefUid": None,
                    "ruleDescPerGroup": [],
                    "autoRuleGroups": [],
                    "autoSourceLayerDefUid": None,
                    "tilesetDefUid": None,
                    "tilePivotX": 0,
                    "tilePivotY": 0,
                    "intGridValues": [
                        {"value": 1, "identifier": "Solid", "color": "#FF0000", "tile": None, "groupUid": 0}
                    ]
                },
                {
                    "__type": "Tiles",
                    "identifier": "Tiles",
                    "type": "Tiles",
                    "uid": 2,
                    "gridSize": 64,
                    "guideGridWid": 0,
                    "guideGridHei": 0,
                    "displayOpacity": 1,
                    "inactiveOpacity": 1,
                    "hideInList": False,
                    "hideFieldsWhenInactive": False,
                    "canSelectWhenInactive": True,
                    "renderInWorldView": True,
                    "pxOffsetX": 0,
                    "pxOffsetY": 0,
                    "parallaxFactorX": 0,
                    "parallaxFactorY": 0,
                    "parallaxScaling": True,
                    "requiredTags": [],
                    "excludedTags": [],
                    "autoTilesetDefUid": None,
                    "ruleDescPerGroup": [],
                    "autoRuleGroups": [],
                    "autoSourceLayerDefUid": None,
                    "tilesetDefUid": 1,
                    "tilePivotX": 0,
                    "tilePivotY": 0,
                    "intGridValues": []
                },
                {
                    "__type": "Tiles",
                    "identifier": "BG_Tiles",
                    "type": "Tiles",
                    "uid": 3,
                    "gridSize": 64,
                    "guideGridWid": 0,
                    "guideGridHei": 0,
                    "displayOpacity": 1,
                    "inactiveOpacity": 1,
                    "hideInList": False,
                    "hideFieldsWhenInactive": False,
                    "canSelectWhenInactive": True,
                    "renderInWorldView": True,
                    "pxOffsetX": 0,
                    "pxOffsetY": 0,
                    "parallaxFactorX": 0,
                    "parallaxFactorY": 0,
                    "parallaxScaling": True,
                    "requiredTags": [],
                    "excludedTags": [],
                    "autoTilesetDefUid": None,
                    "ruleDescPerGroup": [],
                    "autoRuleGroups": [],
                    "autoSourceLayerDefUid": None,
                    "tilesetDefUid": 1,
                    "tilePivotX": 0,
                    "tilePivotY": 0,
                    "intGridValues": []
                },
                {
                    "__type": "Entities",
                    "identifier": "Entities",
                    "type": "Entities",
                    "uid": 4,
                    "gridSize": 64,
                    "guideGridWid": 0,
                    "guideGridHei": 0,
                    "displayOpacity": 1,
                    "inactiveOpacity": 0.5,
                    "hideInList": False,
                    "hideFieldsWhenInactive": False,
                    "canSelectWhenInactive": True,
                    "renderInWorldView": True,
                    "pxOffsetX": 0,
                    "pxOffsetY": 0,
                    "parallaxFactorX": 0,
                    "parallaxFactorY": 0,
                    "parallaxScaling": True,
                    "requiredTags": [],
                    "excludedTags": [],
                    "autoTilesetDefUid": None,
                    "ruleDescPerGroup": [],
                    "autoRuleGroups": [],
                    "autoSourceLayerDefUid": None,
                    "tilesetDefUid": None,
                    "tilePivotX": 0,
                    "tilePivotY": 0,
                    "intGridValues": []
                }
            ],
            "entities": [
                {
                    "identifier": "SpawnSolo",
                    "uid": 100,
                    "tags": [],
                    "exportToToc": False,
                    "allowOutOfLevel": False,
                    "doc": None,
                    "width": 64,
                    "height": 64,
                    "resizableX": False,
                    "resizableY": False,
                    "minWidth": None,
                    "maxWidth": None,
                    "minHeight": None,
                    "maxHeight": None,
                    "keepAspectRatio": False,
                    "tileOpacity": 1,
                    "fillOpacity": 1,
                    "lineOpacity": 1,
                    "hollow": False,
                    "color": "#94D9B3",
                    "renderMode": "Rectangle",
                    "showName": True,
                    "tilesetId": None,
                    "tileRenderMode": "FitInside",
                    "tileRect": None,
                    "uiTileRect": None,
                    "nineSliceBorders": [],
                    "maxCount": 0,
                    "limitScope": "PerLevel",
                    "limitBehavior": "MoveLastOne",
                    "pivotX": 0,
                    "pivotY": 0,
                    "fieldDefs": []
                },
                {
                    "identifier": "CheckpointEnd",
                    "uid": 101,
                    "tags": [],
                    "exportToToc": False,
                    "allowOutOfLevel": False,
                    "doc": None,
                    "width": 64,
                    "height": 64,
                    "resizableX": False,
                    "resizableY": False,
                    "minWidth": None,
                    "maxWidth": None,
                    "minHeight": None,
                    "maxHeight": None,
                    "keepAspectRatio": False,
                    "tileOpacity": 1,
                    "fillOpacity": 1,
                    "lineOpacity": 1,
                    "hollow": False,
                    "color": "#94D9B3",
                    "renderMode": "Rectangle",
                    "showName": True,
                    "tilesetId": None,
                    "tileRenderMode": "FitInside",
                    "tileRect": None,
                    "uiTileRect": None,
                    "nineSliceBorders": [],
                    "maxCount": 0,
                    "limitScope": "PerLevel",
                    "limitBehavior": "MoveLastOne",
                    "pivotX": 0,
                    "pivotY": 0,
                    "fieldDefs": []
                }
            ],
            "tilesets": [
                {
                    "__cWid": 25,
                    "__cHei": 25,
                    "identifier": "Tiles",
                    "uid": 1,
                    "relPath": "assets/textures/tiles/Tiles.png",
                    "embedAtlas": None,
                    "pxWid": 1600,
                    "pxHei": 1600,
                    "tileGridSize": 64,
                    "spacing": 0,
                    "padding": 0,
                    "tags": [],
                    "tagsSourceEnumUid": None,
                    "enumTags": [],
                    "customData": [],
                    "savedSelections": [],
                    "cachedPixelData": None
                }
            ],
            "enums": [],
            "externalEnums": [],
            "levelFields": []
        },
        "levels": [level],
        "worlds": [],
        "dummyWorldIid": "dummy-world-iid"
    }

    os.makedirs("assets/levels", exist_ok=True)
    with open("assets/levels/world.ldtk", "w") as f:
        json.dump(ldtk_data, f, separators=(',', ':'))

if __name__ == "__main__":
    generate_ldtk()
    print("Generated 1000x500 map at assets/levels/world.ldtk")
