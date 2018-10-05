FROM python:3.6-slim
ENV PYTHONUNBUFFERED 1
RUN mkdir /code
WORKDIR /code
ADD requirements.txt /code/
RUN pip install -r requirements.txt
RUN [ "python", "-c", "import nltk; nltk.download('punkt')" ]
ADD . /code/
