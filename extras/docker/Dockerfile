FROM continuumio/miniconda
LABEL org.opencontainers.image.authors="phil.schatzmann@gmail.com"
RUN apt-get update && apt-get install -y git build-essential cmake nano libpulse-dev pulseaudio-utils 
RUN conda create -y -n cling xeus-cling xeus-python jupyterlab pulseaudio -c conda-forge
ENV PATH /opt/conda/envs/cling/bin:/opt/conda/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
RUN mkdir -p /opt/mozzi/workspace && mkdir -p /opt/mozzi/source
COPY mozzi.ipynb /opt/mozzi/workspace/
RUN git clone https://github.com/pschatzmann/mozzi.git /opt/mozzi/source/mozzi
RUN mkdir -p /opt/mozzi/source/mozzi/build
WORKDIR /opt/mozzi/source/mozzi/build
RUN cmake -DCMAKE_INSTALL_PREFIX=/opt/mozzi .. && make install
WORKDIR /opt/mozzi/workspace
CMD jupyter lab --allow-root --ip=0.0.0.0 --port=7890 --no-browser 
