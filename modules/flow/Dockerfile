FROM qore:1.0.0

COPY docker/flow .

WORKDIR /mods

ENV LD_LIBRARY_PATH /lib:${LD_LIBRARY_PATH}

CMD ["/mods/qbus_mod_flow"]

#CMD ["valgrind", "--leak-check=yes", "/mods/qbus_mod_flow"]
