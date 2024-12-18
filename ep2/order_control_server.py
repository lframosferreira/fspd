import sys
import threading
from concurrent import futures
import grpc

import order_control_pb2, order_control_pb2_grpc, stock_control_pb2, stock_control_pb2_grpc

from stock_control_server import Product

current_identifier: int = 1


class OrderControl(order_control_pb2_grpc.OrderControlServicer):
    def __init__(
        self,
        stop_event: threading.Event,
        stock_control_stub: stock_control_pb2_grpc.StockControlStub,
    ):
        self._stop_event: threading.Event = stop_event
        self._orders: dict[int, list[Product]] = dict()
        self._stock_control_stub: stock_control_pb2_grpc.StockControlStub = (
            stock_control_stub
        )

    def create_order(self, request, context):
        item_list: list[tuple[int, int]] = request.item_list
        ret: list[tuple[int, int]] = list()
        for product_id, amount in item_list:
            res = self._stock_control_stub.change_product_amount(
                stock_control_pb2.RequestChangeProductAmount(
                    product_id=product_id, amount=-1 * amount
                )
            )
            match res.status:
                case -1 | -2:
                    ret.append((product_id, res.status))
                case _:
                    ret.append((product_id, min(res.status, 0)))
                    global current_identifier
                    new_identifier: int = current_identifier
                    current_identifier += 1
                    if new_identifier in self._orders:
                        self._orders[new_identifier] += [
                            Product(
                                identifier=product_id, description="", amount=amount
                            )
                        ]
                    else:
                        self._orders[new_identifier] = [
                            Product(
                                identifier=product_id, description="", amount=amount
                            )
                        ]

        return order_control_pb2.ResponseCreateOrder(
            item_list=list(
                map(
                    lambda x: order_control_pb2.ItemResponseCreateOrder(
                        product_id=x[0], status=x[1]
                    ),
                    ret,
                )
            )
        )

    def cancel_order(self, request, context):
        order_id: int = requestorder_id
        if order_id not in self._orders:
            return order_control_pb2.ResponseCancelOrder(status=-1)
        for product in self._orders[order_id]:
            res = self._stock_control_stub.change_product_amount(
                stock_control_pb2.RequestChangeProductAmount(
                    product_id=product._identifier, amount=product._amount
                )
            )

        return order_control_pb2.ResponseCancelOrder(status=0)

    def finish_execution(self, request, context):
        stock_control_finish_value: int = (
            self._stock_control_stub.finish_execution().number_of_existing_products
        )
        self._stop_event.set()
        return order_control_pb2.ResponseFinishExecution(
            stock_control_response=stock_control_finish_value,
            number_of_existing_orders=len(self._orders),
        )


def serve() -> None:
    PORT: str = sys.argv[1]
    STOCK_CONTROL_SERVER: str = sys.argv[2]

    stop_event: threading.Event = threading.Event()
    channel = grpc.insecure_channel(STOCK_CONTROL_SERVER)
    stock_control_stub: stock_control_pb2_grpc.StockControlStub = (
        stock_control_pb2_grpc.StockControlStub(channel)
    )
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    order_control_pb2_grpc.add_OrderControlServicer_to_server(
        OrderControl(stop_event=stop_event, stock_control_stub=stock_control_stub),
        server,
    )
    server.add_insecure_port(f"localhost:{PORT}")
    server.start()
    stop_event.wait()
    server.stop(grace=None)


if __name__ == "__main__":
    serve()
