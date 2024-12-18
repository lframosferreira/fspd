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
        self._orders: dict[int, Product] = dict()
        self._stock_control_stub: stock_control_pb2_grpc.StockControlStub = (
            stock_control_stub
        )

    def create_order(self, request, context):
        item_list: list[tuple[int, int]] = request.item_list
        ret: list[tuple[int, int]] = list()
        for product_id, amount in item_list:
            res = self._stock_control_stub.change_product_amount(
                stock_control_pb2.RequestChangeProductAmount(
                    product_id=product_id, amount=amount
                )
            )
            match res.status:
                case -1 | -2:
                    ret.append((product_id, res.status))
                case _:
                    ret.append((product_id, min(res.status, 0)))
                    global current_identifier
                    new_identifier: int = current_identifier
                    self._orders[new_identifier] = Product(
                        identifier=product_id, description="", amount=amount
                    )
                    current_identifier += 1

        return list(map(lambda x: x, ret))
