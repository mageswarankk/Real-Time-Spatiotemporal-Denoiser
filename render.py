import mitsuba as mi
import numpy as np
import time
import os

mi.set_variant("scalar_rgb")

scene = mi.load_file("classroom/scene.xml")

output_dir = "frames"
os.makedirs(output_dir, exist_ok=True)

num_frames = 300
radius = 10.0
angles = np.linspace(0, 2 * np.pi, num_frames)

def update_camera(scene, angle):
    origin = [-1.0, 1.0, -1.0] 
    target = [radius * np.cos(angle), 1.0, radius * np.sin(angle)] 
    up = [0, 1, 0]

    new_camera = mi.load_dict({
        'type': 'perspective',
        'to_world': mi.ScalarTransform4f.look_at(
            origin=origin,
            target=target,
            up=up
        ),
        'film': {
            'type': 'hdrfilm',
            'width': 1280,
            'height': 720
        }
    })
    return new_camera

start = time.time()

new_camera = update_camera(scene, 0)
image = mi.render(scene=scene, sensor=new_camera, spp=1)
mi.util.write_bitmap(f"{output_dir}/first_frame.png", image[:,:,:3])
last_image = image

for i, angle in enumerate(angles):
    new_camera = update_camera(scene, angle)
    image = mi.render(scene=scene, sensor=new_camera, spp=1)
    mi.util.write_bitmap(f"{output_dir}/frame_{i:03d}.png", image[:,:,:3])
    mi.util.write_bitmap(f"{output_dir}/albedo_{i:03d}.png", image[:,:,3:6])
    mi.util.write_bitmap(f"{output_dir}/depth_{i:03d}.png", image[:,:,6:7]/50)
    mi.util.write_bitmap(f"{output_dir}/normal_{i:03d}.png", abs(image[:,:,7:10]))
    mi.util.write_bitmap(f"{output_dir}/position_{i:03d}.png", abs(image[:,:,10:13])/10 - abs(last_image[:,:,10:13])/10)
    last_image = image
    
end = time.time()
print(f"Total rendering time: {end - start} seconds")

os.system(f"ffmpeg -r 30 -i {output_dir}/frame_%03d.png -vcodec libx264 -pix_fmt yuv420p output.mp4")
print("Video saved as output.mp4")
