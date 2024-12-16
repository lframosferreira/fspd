import sys
from concurrent import futures
import grpc

PORT: int = int(sys.argv[1])


def serve() -> None:
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    server.add_insecure_port(f"localhost:{PORT}")
    server.start()
    server.wait_for_termination()

    if __name__ == "__main__":
        serve()
