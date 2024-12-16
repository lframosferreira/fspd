.PHONY: clean stubs run_serv_estoque run_cli_estoque run_serv_pedidos run_cli_pedidos

clean:
	rm -rf main

stubs:
	test -f main.cpp || echo "main.cpp não encontrado!"

run_serv_estoque:
	python3 stock_control_server.py $(arg1)

run_cli_estoque:
	python3 stock_control_client.py $(arg1)

run_serv_pedidos:
	python3 svc_pedidos.py $(arg1) $(arg2)

run_cli_pedidos:
	python3 cln_pedidos.py $(arg1) $(arg2)

