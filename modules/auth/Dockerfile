FROM qore:1.0.0

COPY docker/auth .

WORKDIR /mods

ENV LD_LIBRARY_PATH /lib:${LD_LIBRARY_PATH}

CMD ["/mods/qbus_mod_auth"]
#CMD ["valgrind", "--leak-check=yes", "/mods/qbus_mod_auth"]
