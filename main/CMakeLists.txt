idf_component_register(
        SRCS main.c network.c http.c
        INCLUDE_DIRS "."
        EMBED_TXTFILES "certs/server.pem" "certs/server_key.pem")
