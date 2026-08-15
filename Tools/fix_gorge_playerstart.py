"""Update PlayerStart Z on existing /Game/Narty/Lvl_Gorge to match arena teleport."""
import unreal

MAP_PATH = "/Game/Narty/Lvl_Gorge"
TARGET = unreal.Vector(-900.0, 0.0, 220.0)


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_subsystem.load_level(MAP_PATH):
        unreal.log_error("Narty: failed to load " + MAP_PATH)
        return

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    if not starts:
        start = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PlayerStart, TARGET, unreal.Rotator(0.0, 0.0, 0.0)
        )
        if start:
            start.set_actor_label("PlayerStart_Narty")
            unreal.log("Narty: spawned PlayerStart at Z=220")
    else:
        for actor in starts:
            actor.set_actor_location(TARGET, False, True)
            unreal.log("Narty: moved PlayerStart to Z=220")

    level_subsystem.save_current_level()
    unreal.log("Narty: Lvl_Gorge PlayerStart aligned")


if __name__ == "__main__":
    main()
