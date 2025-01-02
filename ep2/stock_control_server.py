import sys
import threading
from concurrent import futures
import grpc

import stock_control_pb2, stock_control_pb2_grpc

# variável global utilizada para manter o valor incremental do identificador dos novos produtos
current_identifier: int = 1


# representa um produto disponível no estoque
# todo produto possui um identificador, uma descrição e a quantidade desse produto disponível no estoque
class Product:
    def __init__(self, identifier: int, description: str, amount: int) -> None:
        self._identifier: int = identifier
        self._description: str = description
        self._amount: int = amount

    def increment_amount(self, amount: int) -> None:
        self._amount += amount

    def get_identifier(self) -> int:
        return self._identifier


class StockControl(stock_control_pb2_grpc.StockControlServicer):
    def __init__(self, stop_event: threading.Event):
        self._stop_event: threading.Event = stop_event
        self._products: dict[int, Product] = dict()

    # método para a adição de um produto no estoque. Se o produto já existir, modificamos sua quantidade conforme as regras de negócio
    def add_product(self, request, context):
        description: str = request.description
        amount: int = request.amount
        for identifier, product in self._products.items():
            if product._description == description:
                self._products[identifier].increment_amount(amount=amount)
                return stock_control_pb2.ResponseAddProduct(product_id=identifier)
        global current_identifier
        new_identifier: int = current_identifier
        self._products[new_identifier] = Product(
            identifier=new_identifier, description=description, amount=amount
        )
        current_identifier += 1

        # in either case we should return the product identifier
        return stock_control_pb2.ResponseAddProduct(product_id=new_identifier)

    # método para modificar a quantidade de um produto no estoque
    def change_product_amount(self, request, context):
        product_id: int = request.product_id
        amount: int = request.amount
        if product_id not in self._products:
            return stock_control_pb2.ResponseChangeProductAmount(status=-2)
        new_amount: int = self._products[product_id]._amount + amount
        if new_amount < 0:
            return stock_control_pb2.ResponseChangeProductAmount(status=-1)
        self._products[product_id]._amount = new_amount

        return stock_control_pb2.ResponseChangeProductAmount(status=new_amount)

    # método para a listagem de produtos
    def list_products(self, request, context):
        products = list(
            map(
                lambda x: stock_control_pb2.Product(
                    id=x._identifier, description=x._description, amount=x._amount
                ),
                self._products.values(),
            )
        )
        return stock_control_pb2.ResponseListProducts(products=products)

    # método para terminar a execuçâo
    def finish_execution(self, request, context):
        number_of_existing_products: int = len(self._products)
        self._products.clear()
        self._stop_event.set()
        return stock_control_pb2.ResponseFinishExecution(
            number_of_existing_products=number_of_existing_products
        )


# função para inicializar o servidor
def serve() -> None:
    PORT: str = sys.argv[
        1
    ]  # o primeiro parâmetro da linha de comando é o número da porta em que o servidor deve rodar
    stop_event = threading.Event()

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    stock_control_pb2_grpc.add_StockControlServicer_to_server(
        StockControl(stop_event=stop_event), server
    )
    server.add_insecure_port(f"0.0.0.0:{PORT}")
    server.start()
    stop_event.wait()
    server.stop(grace=None)


if __name__ == "__main__":
    serve()
