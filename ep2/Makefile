.PHONY: clean stubs run_serv_estoque run_cli_estoque run_serv_pedidos run_cli_pedidos

clean:
	rm -rf stock_control_pb2.py stock_control_pb2_grpc.py 

stubs:
	python3 -m grpc_tools.protoc -I. --python_out=. --grpc_python_out=. stock_control.proto 

run_serv_estoque:
	python3 stock_control_server.py $(arg1)

run_cli_estoque:
	python3 stock_control_client.py $(arg1)

run_serv_pedidos:
	python3 svc_pedidos.py $(arg1) $(arg2)

run_cli_pedidos:
	python3 cln_pedidos.py $(arg1) $(arg2)

