"""Run in UE Python. Explicitly apply with optimize(apply=True).

Only the 3 terrain and 8 cliff assets in DreamscapeTower are affected.
Keep their visible mesh, use its triangles for static character collision,
and back up each original package before changing it. No map is resaved.
"""
from pathlib import Path
import shutil
import unreal


def optimize(apply=False):
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    backup = root / 'Saved/CodexBackups/climbing-20261029'
    paths = [f'/Game/DreamscapeSeries/DreamscapeTower/Meshes/Terrain/SM_Terrain_{i:02}' for i in range(1, 4)]
    paths += [f'/Game/DreamscapeSeries/DreamscapeTower/Meshes/Stones/Cliffs/SM_Cliff_{i:02}' for i in range(1, 9)]
    for path in paths:
        mesh = unreal.load_asset(path)
        assert isinstance(mesh, unreal.StaticMesh), path
        body = mesh.get_editor_property('body_setup')
        assert body, path
        before = body.get_editor_property('collision_trace_flag')
        desired = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        if apply and before != desired:
            relative = Path('Content') / (path.removeprefix('/Game/') + '.uasset')
            original, saved = root / relative, backup / relative
            assert original.is_file(), str(original)
            if not saved.exists():
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(original, saved)
            mesh.modify()
            body.modify()
            body.set_editor_property('collision_trace_flag', desired)
            # Notify the owning asset to rebuild collision data before saving.
            mesh.set_editor_property('body_setup', body)
            assert unreal.EditorAssetLibrary.save_loaded_asset(mesh, False), path
            assert body.get_editor_property('collision_trace_flag') == desired
        print('CLIMB_SURFACE', path, before, 'APPLIED' if apply else 'DRY_RUN')


if __name__ == '__main__':
    optimize()
