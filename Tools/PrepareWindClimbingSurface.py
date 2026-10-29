"""Rebuild the wind-only collision change from the user's existing assets.

Run prepare() to inspect, prepare(apply=True) to apply to a saved DemoMap.
Does not ship or modify the original third-party rune-stone mesh.
"""
from pathlib import Path
import shutil
import unreal


def prepare(apply=False):
    source_path = '/Game/DreamscapeSeries/DreamscapeTower/Meshes/Stones/SM_RuneStone_01'
    destination = '/Game/ForestMigration/Climbing/SM_WindRuneClimbSurface'
    map_path = '/Game/DreamscapeSeries/DreamscapeTower/Maps/DemoMap'
    source = unreal.load_asset(source_path)
    assert isinstance(source, unreal.StaticMesh), source_path
    print('WIND_CLIMB', source_path, '->', destination, 'APPLY' if apply else 'DRY_RUN')
    if not apply:
        return
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    assert world.get_path_name().split('.')[0] == map_path, 'Open the saved Dreamscape DemoMap first'
    assert all(p.get_path_name() != map_path for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()), 'Save your map edits first'
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    targets = [a for a in actors if a.get_name() in ('SM_RuneStone_01_WIP_2', 'SM_RuneStone_01_Wind') or a.actor_has_tag('Forest.Region.Wind')]
    assert len(targets) == 1, 'Expected exactly one wind anchor; review custom maps manually'
    components = targets[0].get_components_by_class(unreal.StaticMeshComponent)
    matches = [c for c in components if c.static_mesh and c.static_mesh.get_path_name().split('.')[0] in (source_path, destination)]
    assert len(matches) == 1, 'Wind anchor mesh differs from the expected original or generated copy'
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    relative = Path('Content/DreamscapeSeries/DreamscapeTower/Maps/DemoMap.umap')
    backup = root / 'Saved/CodexBackups/wind-climb-prepare' / relative
    assert (root / relative).is_file()
    if not backup.exists():
        backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / relative, backup)
    desired = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        mesh = unreal.load_asset(destination)
        assert isinstance(mesh, unreal.StaticMesh)
        assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag') == desired, 'Review the existing generated asset; it will not be overwritten'
    else:
        mesh = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination)
        assert mesh
        body = mesh.get_editor_property('body_setup')
        body.set_editor_property('collision_trace_flag', desired)
        mesh.set_editor_property('body_setup', body)
        assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    if matches[0].static_mesh != mesh:
        matches[0].modify()
        matches[0].set_static_mesh(mesh)
        assert unreal.EditorAssetLibrary.save_loaded_asset(world, False)
    print('WIND_CLIMB_READY', matches[0].get_path_name())


if __name__ == '__main__':
    prepare()
