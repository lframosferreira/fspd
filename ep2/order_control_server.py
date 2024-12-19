import sys
import threading
from concurrent import futures
import grpc

import order_control_pb2, order_control_pb2_grpc, stock_control_pb2, stock_control_pb2_grpc

from stock_control_server import Product

# variável global para incrementar o identificador dos novos pedidos
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

    # método para criação de um pedido no servidor
    def create_order(self, request, context):
        item_list: list = request.item_list
        ret: list[tuple[int, int]] = list()
        new_order_created: bool = False
        for item in item_list:
            product_id: int = item.product_id
            amount: int = item.amount
            res = self._stock_control_stub.change_product_amount(
                stock_control_pb2.RequestChangeProductAmount(
                    product_id=product_id, amount=-1 * amount
                )
            )
            match res.status:
                case -1 | -2:
                    ret.append((product_id, res.status))
                case _:
                    new_order_created = True
                    ret.append((product_id, min(res.status, 0)))
                    global current_identifier
                    new_identifier: int = current_identifier
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
        if new_order_created:
            current_identifier += 1

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

    # método utilizado para cancelar um pedido
    def cancel_order(self, request, context):
        order_id: int = request.order_id
        if order_id not in self._orders:
            return order_control_pb2.ResponseCancelOrder(status=-1)
        for product in self._orders[order_id]:
            res = self._stock_control_stub.change_product_amount(
                stock_control_pb2.RequestChangeProductAmount(
                    product_id=product._identifier, amount=product._amount
                )
            )
        del self._orders[order_id]

        return order_control_pb2.ResponseCancelOrder(status=0)

    # método utilizado para finalizar a execuçaõ do servidor que manuseia os pedidos
    def finish_execution(self, request, context):
        stock_control_finish_value: int = self._stock_control_stub.finish_execution(
            stock_control_pb2.RequestFinishExecution()
        ).number_of_existing_products
        self._stop_event.set()
        return order_control_pb2.ResponseFinishExecution(
            stock_control_response=stock_control_finish_value,
            number_of_existing_orders=len(self._orders),
        )


# função utilizada para inicializar o servidor de pedidos que começará a ser executado
# na porta passada como primeiro argumento da linha de comando
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
