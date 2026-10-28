"""Run in UE's Python console after importing both animation packs at their original paths.

Creates the action copies without modifying the downloaded animation sequences.
No third-party assets are distributed with this script. Existing action copies
must match the expected skeleton and length; unknown assets are never replaced.
"""
import unreal

target = unreal.load_asset('/Game/ChengYi/Mesh/SKEL_ChengYi_Skeleton')
assert target, 'Import the ChengYi character first'
target_pose = target.get_reference_pose()
target_bones = set(map(str, target_pose.get_bone_names()))
source_skeletons = [
    '/Game/LaddersAndLedges/Mannequin/Character/Mesh/UE4_Mannequin_Skeleton',
    '/Game/ResourceGatheringAnimationPack/EpicContent/Mannequin/Character/Mesh/UE4_Mannequin_Skeleton',
]
for path in source_skeletons:
    skeleton = unreal.load_asset(path)
    assert skeleton, 'Missing source skeleton: ' + path
    pose = skeleton.get_reference_pose()
    assert set(map(str, pose.get_bone_names())) <= target_bones, 'Different bone names: ' + path
    for bone in ('pelvis', 'spine_01', 'upperarm_l', 'upperarm_r', 'thigh_l', 'thigh_r'):
        a = target_pose.get_ref_bone_pose(bone).rotation
        b = pose.get_ref_bone_pose(bone).rotation
        assert abs(a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w) > .999, 'Retarget this different bind pose first'
    target.add_compatible_skeleton(skeleton)
assert unreal.EditorAssetLibrary.save_loaded_asset(target, False)

selected = {}
for stance in ('Crouched', 'Standing'):
    for suffix in ('Start', '', 'End'):
        selected['Gather' + stance + (suffix or 'Loop')] = (
            '/Game/ResourceGatheringAnimationPack/Animations/Pickup' + stance + 'Right' + suffix)
for name, source in (
    ('ClimbIdle', 'climb_idle_cfoot'), ('ClimbUp', 'climb_U'), ('ClimbDown', 'climb_D'),
    ('ClimbLeft', 'climb_L'), ('ClimbRight', 'climb_R'), ('ClimbTop', 'climb_up_out')):
    selected[name] = '/Game/LaddersAndLedges/Animations/HangingAndLedges/' + source

for name, path in selected.items():
    source = unreal.load_asset(path)
    assert isinstance(source, unreal.AnimSequence) and source.get_skeleton(), path
    destination = '/Game/Characters/Actions/AS_' + name
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        asset = unreal.load_asset(destination)
        assert isinstance(asset, unreal.AnimSequence), destination
        assert asset.get_skeleton() == source.get_skeleton(), destination
        assert abs(asset.get_play_length() - source.get_play_length()) < .001, destination
        # Existing copies are inspected, not overwritten or silently reconfigured.
        assert asset.get_editor_property('force_root_lock'), destination
        assert not asset.get_editor_property('enable_root_motion'), destination
    else:
        asset = unreal.EditorAssetLibrary.duplicate_asset(path, destination)
        assert asset, destination
        asset.set_editor_property('enable_root_motion', False)
        asset.set_editor_property('force_root_lock', True)
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    print(destination, asset.get_play_length(), asset.get_skeleton().get_path_name())
print('Prepared 12 player action sequences; no gameplay was run.')
