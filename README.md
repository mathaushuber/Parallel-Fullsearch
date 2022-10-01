# Parallel-FullSearch
Aiming to bring an efficient method of the Full Search algorithm, through the massive parallelism available in current GPGPU devices.
(i) The parallel solution for multi-core GPP using OpenMP library; (ii) The distributed solution for cluster/grid machines using Message Passing Library Interfaces (MPI).

In the file, for each frame, there are the data of the Y, Cb and Cr channels: The luminance channel has all pixels (640x360). The Cb and Cr channels have 
only 1/4 of the pixels (therefore, 320x180). Therefore, each frame must be read by reading a 640x360 matrix (Y Channel), and then two 320x180 matrices
(Cb and Cr Channels).
