
from django.test import TestCase


class InvoiceTest(TestCase):

    def setUp(self):
        self.url = '/api/invoice/'

    def test_create(self):

        file = open("invoice/test.pdf", 'rb')
        response = self.client.post(self.url, {'file': file}, format='multipart')
        print(response)
        self.assertEqual(response.status_code, 200)
