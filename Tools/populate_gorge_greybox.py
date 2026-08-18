"""Place editable greybox + story actors into /Game/Narty/Lvl_Gorge.

Idempotent: deletes previous Grey_* / Story_* / Point_* actors, keeps Sun/Sky/PlayerStart.
"""
import unreal

MAP_PATH = "/Game/Narty/Lvl_Gorge"
ORIGIN = unreal.Vector(0.0, 0.0, 100.0)
GREY_TAG = "NartyGreybox"
STORY_TAG = "NartyStory"


def _world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def _rel(x, y, z):
    return ORIGIN + unreal.Vector(x, y, z)


def _destroy_prefixed(world, prefixes):
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
    removed = 0
    for actor in actors:
        label = actor.get_actor_label()
        if any(label.startswith(prefix) for prefix in prefixes):
            actor.destroy_actor()
            removed += 1
    unreal.log("Narty: removed {} previous greybox/story actors".format(removed))


def _spawn_cube(label, location, scale, color):
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, location, unreal.Rotator(0.0, 0.0, 0.0)
    )
    if not actor:
        unreal.log_error("Narty: failed to spawn " + label)
        return None

    actor.set_actor_label(label)
    actor.set_folder_path("Narty/Greybox")
    actor.set_actor_scale3d(scale)
    actor.set_editor_property("tags", [GREY_TAG])

    mesh = actor.static_mesh_component
    mesh.set_static_mesh(cube)
    mesh.set_mobility(unreal.ComponentMobility.STATIC)
    mesh.set_collision_profile_name("BlockAll")

    try:
        dyn = mesh.create_dynamic_material_instance(0)
        if dyn:
            linear = unreal.LinearColor(color[0], color[1], color[2], 1.0)
            dyn.set_vector_parameter_value("Color", linear)
            dyn.set_vector_parameter_value("BaseColor", linear)
    except Exception as exc:
        unreal.log_warning("Narty: material tint skipped for {}: {}".format(label, exc))

    return actor


def _spawn_class(script_path, label, location, rotator, folder):
    cls = unreal.load_class(None, script_path)
    if not cls:
        unreal.log_error("Narty: could not load " + script_path)
        return None

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, location, rotator)
    if not actor:
        unreal.log_error("Narty: failed to spawn " + label)
        return None

    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.set_editor_property("tags", [STORY_TAG])
    return actor


def _spawn_point(label, location):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.TargetPoint, location, unreal.Rotator(0.0, 0.0, 0.0)
    )
    if actor:
        actor.set_actor_label(label)
        actor.set_folder_path("Narty/Points")
    return actor


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level(MAP_PATH):
        unreal.log_error("Narty: failed to load " + MAP_PATH)
        return

    world = _world()
    if not world:
        unreal.log_error("Narty: no editor world")
        return

    _destroy_prefixed(world, ("Grey_", "Story_", "Point_"))

    for start in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart):
        start.set_editor_property("tags", ["NartyPlayerStart"])
        label = start.get_actor_label()
        if "Narty" not in label:
            start.set_actor_label("PlayerStart_Narty")

    rock = (0.30, 0.27, 0.24)
    wall = (0.25, 0.22, 0.20)
    floor = (0.18, 0.16, 0.14)
    step = (0.24, 0.21, 0.18)

    blocks = [
        ("Grey_Floor", _rel(0, 0, -50), unreal.Vector(42, 24, 1), floor),
        ("Grey_WallLeft", _rel(200, -900, 400), unreal.Vector(38, 1.5, 10), wall),
        ("Grey_WallRightA", _rel(-1200, 900, 400), unreal.Vector(16, 1.5, 10), wall),
        ("Grey_WallRightB", _rel(1200, 900, 400), unreal.Vector(24, 1.5, 10), wall),
        ("Grey_WallBack", _rel(-1600, 0, 400), unreal.Vector(1.5, 18, 10), (0.20, 0.18, 0.16)),
        ("Grey_WallFrontL", _rel(1900, -380, 400), unreal.Vector(1.4, 6, 10), (0.22, 0.20, 0.17)),
        ("Grey_WallFrontR", _rel(1900, 380, 400), unreal.Vector(1.4, 6, 10), (0.22, 0.20, 0.17)),
        ("Grey_RockA", _rel(-400, -280, 80), unreal.Vector(2, 2, 3), rock),
        ("Grey_RockB", _rel(200, 300, 60), unreal.Vector(2.5, 1.8, 2.2), (0.28, 0.25, 0.22)),
        ("Grey_RockC", _rel(700, -260, 70), unreal.Vector(1.8, 2.2, 2.8), (0.32, 0.28, 0.24)),
        ("Grey_Step1", _rel(1200, 0, 20), unreal.Vector(3.5, 4, 0.6), step),
        ("Grey_Step2", _rel(1340, 0, 70), unreal.Vector(3.5, 4, 0.6), step),
        ("Grey_Step3", _rel(1480, 0, 120), unreal.Vector(3.5, 4, 0.6), step),
        ("Grey_Ledge", _rel(1680, 0, 160), unreal.Vector(8, 10, 0.8), (0.20, 0.18, 0.15)),
        ("Grey_FireRailBack", _rel(2080, 0, 360), unreal.Vector(1.2, 10, 5), wall),
        ("Grey_FireRailRight", _rel(1680, 520, 360), unreal.Vector(8, 1.2, 5), wall),
		("Grey_FireRailLeft", _rel(1680, -520, 360), unreal.Vector(8, 1.2, 5), wall),
        ("Grey_HearthApron", _rel(-600, 500, -50), unreal.Vector(20, 20, 1), (0.17, 0.15, 0.13)),
        ("Grey_TrialPathFloor", _rel(-200, 1000, -50), unreal.Vector(16, 22, 1), (0.17, 0.15, 0.13)),
        ("Grey_NykhasJoin", _rel(-350, 250, -50), unreal.Vector(16, 12, 1), (0.16, 0.14, 0.12)),
        ("Grey_FireStepR1", _rel(1380, 280, 20), unreal.Vector(5, 4, 0.6), step),
        ("Grey_FireStepR2", _rel(1520, 280, 80), unreal.Vector(5, 4, 0.6), step),
        ("Grey_FireStepR3", _rel(1640, 220, 130), unreal.Vector(5, 4, 0.6), step),
        ("Grey_FireStepL1", _rel(1380, -280, 20), unreal.Vector(5, 4, 0.6), step),
        ("Grey_FireStepL2", _rel(1520, -280, 80), unreal.Vector(5, 4, 0.6), step),
        ("Grey_FireStepL3", _rel(1640, -220, 130), unreal.Vector(5, 4, 0.6), step),
        ("Grey_NykhasFloor", _rel(-200, 0, -20), unreal.Vector(6, 6, 0.4), (0.16, 0.14, 0.12)),
    ]

    for label, loc, scale, color in blocks:
        _spawn_cube(label, loc, scale, color)

    story = [
        (
            "/Script/Narty.NartyForgeActor",
            "Story_Forge",
            _rel(1100, 0, 40),
            unreal.Rotator(0, 180, 0),
        ),
        (
            "/Script/Narty.NartyTrainingDummy",
            "Story_DummyA",
            _rel(200, -120, 90),
            unreal.Rotator(0, 0, 0),
        ),
        (
            "/Script/Narty.NartyTrainingDummy",
            "Story_DummyB",
            _rel(450, 140, 90),
            unreal.Rotator(0, 0, 0),
        ),
        (
            "/Script/Narty.NartySettlementHearthActor",
            "Story_Hearth",
            _rel(-750, 180, 40),
            unreal.Rotator(0, -90, 0),
        ),
        (
            "/Script/Narty.NartyMountainFireActor",
            "Story_MountainFire",
            _rel(1580, 0, 230),
            unreal.Rotator(0, 180, 0),
        ),
        (
            "/Script/Narty.NartyUatsamongaCup",
            "Story_Uatsamonga",
            _rel(-200, 0, 50),
            unreal.Rotator(0, 180, 0),
        ),
        (
            "/Script/Narty.NartyHeroTrialSite",
            "Story_HeroTrial",
            _rel(-200, 1100, 0),
            unreal.Rotator(0, -90, 0),
        ),
        (
            "/Script/Narty.NartyFinaleSite",
            "Story_Finale",
            _rel(-1100, -250, 40),
            unreal.Rotator(0, 90, 0),
        ),
        (
            "/Script/Narty.NartyUaigEnemy",
            "Story_Uaig",
            _rel(750, 200, 130),
            unreal.Rotator(0, 180, 0),
        ),
        (
            "/Script/Narty.NartyUaigEnemy",
            "Story_UaigB",
            _rel(880, -180, 130),
            unreal.Rotator(0, 180, 0),
        ),
    ]

    for path, label, loc, rot in story:
        _spawn_class(path, label, loc, rot, "Narty/Story")

    points = [
        ("Point_Nykhas", _rel(-200, 0, 50)),
        ("Point_Forge", _rel(1100, 0, 40)),
        ("Point_Hearth", _rel(-750, 180, 40)),
        ("Point_MountainFire", _rel(1580, 0, 230)),
        ("Point_HeroTrial", _rel(-200, 1100, 0)),
        ("Point_Finale", _rel(-1100, -250, 40)),
        ("Point_River", _rel(1580, 0, 50)),
        ("Point_Uaig", _rel(750, 200, 130)),
        ("Point_UaigB", _rel(880, -180, 130)),
    ]
    for label, loc in points:
        _spawn_point(label, loc)

    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0.0, 0.0, 200.0), unreal.Rotator()
    )
    if fog:
        fog.set_actor_label("Grey_HeightFog")
        fog.set_folder_path("Narty/Greybox")
        fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fog_comp:
            fog_comp.set_editor_property("fog_density", 0.012)
            fog_comp.set_editor_property("fog_height_falloff", 0.12)
            fog_comp.set_editor_property("start_distance", 600.0)

    for sun in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.DirectionalLight):
        sun.set_actor_rotation(unreal.Rotator(-42.0, 35.0, 0.0), False)
        sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            sun_comp.set_intensity(12.0)
            sun_comp.set_light_color(unreal.LinearColor(1.0, 0.94, 0.82, 1.0))
        break

    sky_light = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 400.0), unreal.Rotator()
    )
    if sky_light:
        sky_light.set_actor_label("Grey_SkyLight")
        sky_light.set_folder_path("Narty/Greybox")
        sky_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
        if sky_comp:
            sky_comp.set_intensity(1.25)

    level_subsystem.save_current_level()
    unreal.log("Narty: Lvl_Gorge greybox + story points placed")


if __name__ == "__main__":
    main()
