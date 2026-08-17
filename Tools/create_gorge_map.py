"""Create /Game/Narty/Lvl_Gorge with NartyGameMode + PlayerStart."""
import unreal

MAP_PATH = "/Game/Narty/Lvl_Gorge"
GAME_MODE_PATH = "/Script/Narty.NartyGameMode"


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

    parent = "/Game/Narty"
    if not asset_subsystem.does_directory_exist(parent):
        asset_subsystem.make_directory(parent)

    # New blank level (overwrites if exists)
    ok = level_subsystem.new_level(MAP_PATH)
    if not ok:
        unreal.log_error("Narty: failed to create level " + MAP_PATH)
        return

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        unreal.log_error("Narty: no editor world after new_level")
        return

    world_settings = world.get_world_settings()
    gm_class = unreal.load_class(None, GAME_MODE_PATH)
    if not gm_class:
        unreal.log_error("Narty: could not load " + GAME_MODE_PATH)
        return

    world_settings.set_editor_property("default_game_mode", gm_class)

    # Clear existing PlayerStarts if any, then spawn one
    existing = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    for actor in existing:
        actor.destroy_actor()

    start = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart,
        # Match ANartyPrototypeArena::GetPlayerStartTransform (arena at Z=100 + offset Z=120).
        unreal.Vector(-900.0, 0.0, 220.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if start:
        start.set_actor_label("PlayerStart_Narty")
        start.set_editor_property("tags", ["NartyPlayerStart"])

    # Soft floor light so PIE isn't pitch black before arena builds
    light = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 500.0),
        unreal.Rotator(-50.0, 30.0, 0.0),
    )
    if light:
        light.set_actor_label("Sun_Narty")
        light.set_actor_rotation(unreal.Rotator(-42.0, 35.0, 0.0), False)
        sun_comp = light.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            sun_comp.set_intensity(12.0)
            sun_comp.set_light_color(unreal.LinearColor(1.0, 0.94, 0.82, 1.0))

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if sky:
        sky.set_actor_label("Sky_Narty")

    level_subsystem.save_current_level()
    unreal.log("Narty: created " + MAP_PATH + " with NartyGameMode")


if __name__ == "__main__":
    main()
