# Real-Time-Spatiotemporal-Denoiser
*Spatiotemporal Joint Bilateral Denoising algorithm to enable Real-Time Ray Tracing (by denoising 1 SPP images)*

This project involved rendering a 3D scene with low SPP ray tracing and applying a joint bilateral filter for denoising in real-time. I began with setting up and rendering a classroom scene in Mitsuba 3. I rendered a total of 300 frames, each with a rotating camera to capture various viewpoints. For each frame, multiple buffer outputs were saved, including albedo, depth, normal, and motion vectors, which were essential for the denoising process. The motion vectors were calculated by subtracting the previous frame’s position buffer from the current frame’s position buffer. I utilized GLFW and GLAD to initialize an OpenGL context. The rendered frames and their respective buffers were loaded as textures into the OpenGL application. The noisy images were mapped onto a single quad, and the filter was implemented in the fragment shader.

The spatial component joint bilateral filter employed the following information: spatial distance, color similarity, albedo, depth, and normal. The shader calculated a weighted sum of neighboring pixels, where the weights were determined based on these factors, ensuring that similar pixels had more influence on the final color. To optimize this, I employed the multiple passes technique, where I denoised once vertically and once horizontally. I used an analytical pseudorandom generator to add some random movement to avoid grid artifacts in the final product. I also experimented with implementing progressively growing sizes, but I preferred the look of the final product without it.

Additionally, the shader incorporated a temporal component by blending the current frame with the previous denoised frame, guided by motion vectors to account for any movement. This temporal filtering helped in further reducing noise while maintaining temporal coherence across frames. In the video, you can see multiple techniques used in temporal clamping. I observed the lagging caused when no clamping is used. However, when I clamped the temporal output to the pixel in the noisy image, I didn’t like the trade-off between noise and lagging. There was too much noise. So, I also tried clamping the temporal output to the pixel in the spatially filtered image. While this gave the final product the same artifacts produced by just spatial filtering, I think it looked more visually pleasing.

Implementing a joint bilateral filter in GLSL deepened my understanding of shader programming and the utility of various buffers in denoising. Additionally, the project helped me experiment with other real-time rendering techniques, such as frame rate control, texture management, and, most importantly, performance optimization.

Watch the video demonstration below:
[![Watch the video](https://img.youtube.com/vi/MqXORUgg0ho/maxresdefault.jpg)](https://www.youtube.com/watch?v=MqXORUgg0ho)
