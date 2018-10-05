from django.conf.urls import url
from .views import InvoiceView


urlpatterns = [
    url(r'^invoice/', InvoiceView.as_view())
]
