FROM qore:1.0.0

COPY docker/jobs .

WORKDIR /mods

#CMD ["/mods/qbus_mod_jobs"]

CMD ["valgrind", "--leak-check=yes", "/mods/qbus_mod_jobs"]