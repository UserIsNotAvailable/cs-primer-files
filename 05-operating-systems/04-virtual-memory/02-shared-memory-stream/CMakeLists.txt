add_executable(server server.c shm_blocking_queue.h shm_blocking_queue.c)
add_executable(client client.c shm_blocking_queue.h shm_blocking_queue.c)