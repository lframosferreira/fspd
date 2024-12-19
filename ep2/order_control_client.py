from __future__ import print_function
import os
import sys
import grpc

import order_control_pb2, order_control_pb2_grpc, stock_control_pb2, stock_control_pb2_grpc
from stock_control_server import Product


# função para criar un pedido
def create_order(
    order_control_stub: order_control_pb2_grpc.OrderControlStub,
    list_of_orders: list[tuple[int, int]],
) -> None:
    response = order_control_stub.create_order(
        order_control_pb2.RequestCreateOrder(
            item_list=list(
                map(
                    lambda x: order_control_pb2.ItemRequestCreateOrder(
                        product_id=x[0], amount=x[1]
                    ),
                    list_of_orders,
                )
            )
        )
    ).item_list
    for item in response:
        print(f"{item.product_id} {item.status}")


# função para cancelar um pedido
def cancel_order(
    order_control_stub: order_control_pb2_grpc.OrderControlStub, order_id: int
) -> None:
    response = order_control_stub.cancel_order(
        order_control_pb2.RequestCancelOrder(order_id=order_id)
    )
    print(response.status)


# função para finalizar a execução do servidor de estoque e de pedidos
def finish_execution(
    order_control_stub: order_control_pb2_grpc.OrderControlStub,
) -> None:
    response = order_control_stub.finish_execution(
        order_control_pb2.RequestFinishExecution()
    )
    print(f"{response.stock_control_response} {response.number_of_existing_orders}")


# função para ler os valores da entrada padrão, lendo linha por linha
def process_input(
    order_control_stub: order_control_pb2_grpc.OrderControlStub,
    stock_control_stub: stock_control_pb2_grpc.StockControlStub,
) -> None:
    for line in sys.stdin:
        if len(line) > 0:
            line = line[:-1]
        if not line:
            continue
        args: list[str] = line.split(" ")
        match args[0]:
            case "P":
                list_of_orders: list[tuple[int, int]] = [
                    (int(args[i]), int(args[i + 1])) for i in range(1, len(args[1:]), 2)
                ]
                create_order(
                    order_control_stub=order_control_stub, list_of_orders=list_of_orders
                )
            case "X":
                order_id: int = int(args[1])
                cancel_order(order_control_stub=order_control_stub, order_id=order_id)
            case "T":
                finish_execution(order_control_stub=order_control_stub)
            case _:
                continue


def main() -> None:
    STOCK_CONTROL_SERVER_ADDRESS: str = sys.argv[1]
    ORDER_CONTROL_SERVER_ADDRESS: str = sys.argv[2]

    stock_control_channel: grpc.Channel = grpc.insecure_channel(
        STOCK_CONTROL_SERVER_ADDRESS
    )
    stock_control_stub: stock_control_pb2_grpc.StockControlStub = (
        stock_control_pb2_grpc.StockControlStub(stock_control_channel)
    )

    list_of_products_in_stock: list[Product] = stock_control_stub.list_products(
        stock_control_pb2.RequestListProducts()
    ).products
    for product in list_of_products_in_stock:
        print(f"{product.id} {product.amount} {product.description}")

    order_control_channel: grpc.Channel = grpc.insecure_channel(
        ORDER_CONTROL_SERVER_ADDRESS
    )
    order_control_stub: order_control_pb2_grpc.OrderControlStub = (
        order_control_pb2_grpc.OrderControlStub(order_control_channel)
    )

    process_input(
        order_control_stub=order_control_stub, stock_control_stub=stock_control_stub
    )

    order_control_channel.close()
    stock_control_channel.close()


if __name__ == "__main__":
    main()
