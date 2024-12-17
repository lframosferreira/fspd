import sys
import threading
from concurrent import futures
import grpc

import stock_control_pb2, stock_control_pb2_grpc

current_identifier: int = 1


class Product:
    def __init__(self, amount: int) -> None:
        global current_identifier
        self._identifier: int = current_identifier
        current_identifier += 1
        self._amount: int = amount

    def increment_amount(self, amount: int) -> None:
        self._amount += amount

    def get_identifier(self) -> int:
        return self._identifier


class StockControl(stock_control_pb2_grpc.StockControlServicer):
    def __init__(self, stop_event: threading.Event):
        self._stop_event: threading.Event = stop_event
        self._products: dict[str, Product] = dict()

    def add_product(self, request, context):
        description: str = request.description
        amount: int = request.amount
        if description in self._products:
            self._products[description].increment_amount(amount=amount)
        else:
            self._products[description] = Product(amount=amount)

        # in either case we should return the product identifier
        return stock_control_pb2.ResponseAddProduct(
            product_id=self._products[description].get_identifier()
        )


def serve() -> None:
    PORT: str = sys.argv[1]
    stop_event = threading.Event()

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    server.add_insecure_port(f"localhost:{PORT}")
    stock_control_pb2_grpc.add_StockControlServicer_to_server(
        StockControl(stop_event=stop_event), server
    )
    server.start()
    stop_event.wait()
    server.stop(grace=None)


if __name__ == "__main__":
    serve()
