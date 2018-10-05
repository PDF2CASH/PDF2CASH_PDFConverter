#!/bin/bash

apps="$(ls -d */ | grep PDF2CASH_PDFToInvoice -v)"

for app in ${apps[@]}
do
    rm -rf $app/migrations/0*
    sudo rm -rf $app/migrations/__pycache__
done

rm -rf mydatabase
python manage.py makemigrations
python manage.py migrate