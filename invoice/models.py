from django.db import models


class Invoice(models.Model):

    file = models.FileField(upload_to='invoice/images/', blank=True)

    text = models.TextField()

    number = models.CharField(max_length=9, blank=True)

    operation_nature = models.CharField(max_length=255, blank=True)  # NATUREZA DA OPERAÇÃO

    authorization_protocol = models.CharField(max_length=255, blank=True)  # PROTOCOLO DE AUTORIZAÇÃO

    access_key = models.CharField(max_length=44, unique=True)  # CHAVE DE ACESSO

    state_registration = models.CharField(max_length=255, blank=True)  # INSCRIÇÃO ESTADUAL

    receiver = models.ForeignKey(
        'invoice.Receiver',
        related_name='invoices',
        on_delete=models.CASCADE
    )

    seller = models.ForeignKey(
        'invoice.Seller',
        related_name='invoices',
        on_delete=models.CASCADE
    )

    emission_date = models.DateField(blank=True)

    entry_exit_datetime = models.DateTimeField(blank=True)

    total_products_value = models.CharField(max_length=30, blank=True)

    total_invoice_value = models.CharField(max_length=30, blank=True)

    freight_value = models.CharField(max_length=30, blank=True)

    icms_value = models.CharField(max_length=30, blank=True)

    discount_value = models.CharField(max_length=30, blank=True)


class Seller(models.Model):

        name = models.CharField(max_length=255, blank=True)

        cnpj = models.CharField(max_length=14, blank=True)

        cep = models.CharField(max_length=8, blank=True)

        uf_code = models.IntegerField(blank=True)


class Receiver(models.Model):

        name = models.CharField(max_length=255, blank=True)

        cpf_cnpj = models.CharField(max_length=14, unique=True)

        address = models.CharField(max_length=255, blank=True)

        neighborhood = models.CharField(max_length=255, blank=True)

        cep = models.CharField(max_length=8, blank=True)

        county = models.CharField(max_length=255, blank=True)

        uf = models.CharField(max_length=2, blank=True)

        phone = models.CharField(max_length=13, blank=True)


class Product_Service(models.Model):

        code = models.CharField(max_length=100)

        description = models.CharField(max_length=255)

        qtd = models.IntegerField()

        unity_value = models.CharField(max_length=30)

        total_value = models.CharField(max_length=30)

        discount_value = models.CharField(max_length=30)
