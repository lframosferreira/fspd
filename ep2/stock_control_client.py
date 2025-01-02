import enum
import os
import sys
import grpc

import stock_control_pb2, stock_control_pb2_grpc


# função para o cliente fazer uma requisição de adição de produto no estoque
def add_product(
    stub: stock_control_pb2_grpc.StockControlStub, amount: int, description: str
) -> None:
    response = stub.add_product(
        stock_control_pb2.RequestAddProduct(amount=amount, description=description)
    )
    print(response.product_id)


# função para o cliente fazer uma requisição de mudar a quantidade de um produto no estoque
def change_product_amount(
    stub: stock_control_pb2_grpc.StockControlStub, product_id: int, amount: int
) -> None:
    response = stub.change_product_amount(
        stock_control_pb2.RequestChangeProductAmount(
            product_id=product_id, amount=amount
        )
    )
    print(response.status)


# função para o cliente fazer uma requisição para listar os produtos
def list_products(stub: stock_control_pb2_grpc.StockControlStub) -> None:
    response = stub.list_products(stock_control_pb2.RequestListProducts())
    for product in response.products:
        print(f"{product.id} {product.description} {product.amount}")


# função para o cliente fazer uma requisição para finalizar a execução
def finish_execution(stub: stock_control_pb2_grpc.StockControlStub) -> None:
    response = stub.finish_execution(stock_control_pb2.RequestFinishExecution())
    print(response.number_of_existing_products)


# função que processa a entrada vindo da entrada padrão
# lê as linhas vindas de stdin
def process_input(stub: stock_control_pb2_grpc.StockControlStub) -> None:
    for line in sys.stdin:
        # remove endline \n
        if len(line) > 0:
            line = line[:-1]
        if not line:
            continue
        args: list[str] = line.split(" ")

        if args[0] == "P":
            add_product(stub=stub, amount=int(args[1]), description=" ".join(args[2:]))
        elif args[0] == "Q":
            change_product_amount(
                stub=stub, product_id=int(args[1]), amount=int(args[2])
            )
        elif args[0] == "L":
            list_products(stub=stub)
        elif args[0] == "F":
            finish_execution(stub=stub)
            return
        else:
            continue


# função main que estabelece a conexão com o servidor
def main() -> None:
    ADDRESS: str = sys.argv[1]
    channel: grpc.Channel = grpc.insecure_channel(ADDRESS)
    stub: stock_control_pb2_grpc.StockControlStub = (
        stock_control_pb2_grpc.StockControlStub(channel)
    )
    process_input(stub)

    channel.close()


if __name__ == "__main__":
    main()
