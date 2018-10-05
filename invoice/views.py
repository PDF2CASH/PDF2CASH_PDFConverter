from rest_framework.response import Response
from .models import Seller, Receiver
from .serializers import InvoiceSerializer
import PyPDF2
from nltk.tokenize import word_tokenize
import re
from pdfminer.pdfinterp import PDFResourceManager, PDFPageInterpreter
from pdfminer.converter import TextConverter
from pdfminer.layout import LAParams
from pdfminer.pdfpage import PDFPage
from io import StringIO
from rest_framework.parsers import MultiPartParser
from rest_framework.views import APIView


def convert_pdf_to_txt(file):

    rsrcmgr = PDFResourceManager()
    retstr = StringIO()
    codec = 'utf-8'
    laparams = LAParams()
    device = TextConverter(rsrcmgr, retstr, codec=codec, laparams=laparams)
    fp = file
    interpreter = PDFPageInterpreter(rsrcmgr, device)
    password = ""
    maxpages = 0
    caching = True
    pagenos = set()
    for page in PDFPage.get_pages(
        fp,
        pagenos,
        maxpages=maxpages,
        password=password,
        caching=caching,
        check_extractable=True
    ):

        interpreter.process_page(page)

    text = retstr.getvalue()

    device.close()
    retstr.close()
    return text


def search_create_receiver(cpnj_cpf_receiver, text):

    if Receiver.objects.filter(cpf_cnpj=cpnj_cpf_receiver).count() == 1:
            receiver = Receiver.objects.get(cpf_cnpj=cpnj_cpf_receiver)
    else:
        # Procurar os atributos do Receiver

        name_receiver = re.search(r'NOME\/RAZÃO\s+SOCIAL\s+([\w+| |\.]+\w)', text, re.M | re.I)

        name_receiver = str(name_receiver.group()).replace('NOME/RAZÃO SOCIAL', '')
        name_receiver = name_receiver.replace('\n', '')

        print("---------------")
        print(name_receiver)
        print("---------------")

        address_receiver = re.search(r'ENDEREÇO\s+(.+)', text, re.M | re.I)
        address_receiver = str(address_receiver.group()).replace('ENDEREÇO', '')
        address_receiver = address_receiver.replace('\n', '')

        print("---------------")
        print(address_receiver)
        print("---------------")

        neighborhood_receiver = re.search(r'BAIRRO\/DISTRITO\s+(.+)', text, re.M | re.I)
        neighborhood_receiver = str(neighborhood_receiver.group()).replace('BAIRRO/DISTRITO', '')
        neighborhood_receiver = neighborhood_receiver.replace('\n', '')

        print("---------------")
        print(neighborhood_receiver)
        print("---------------")

        cep_receiver = re.search(
            r'DESTINATÁRIO[\W|\w]+CEP[\W|\w]+\s(\d{5}\-\d{3}|\d{2}\.\d{3}\-\d{3}|\d{8})\s[\W|\w]+CÁLCULO DO IMPOSTO',
            text,
            re.M | re.I
        )
        cep_receiver = str(cep_receiver.group())
        cep_receiver = re.search(r'CEP[\W|\w]+\s(\d{5}\-\d{3}|\d{2}\.\d{3}\-\d{3}|\d{8})', cep_receiver, re.M | re.I)
        cep_receiver = str(cep_receiver.group()).replace('\n', '')
        cep_receiver = cep_receiver.replace('-', '')
        cep_receiver = cep_receiver.replace('.', '')
        cep_receiver = cep_receiver.replace('/', '')
        cep_receiver = cep_receiver.replace(' ', '')
        cep_receiver = cep_receiver.replace('CEP', '')
        cep_receiver = cep_receiver[-8:]

        print("---------------")
        print(cep_receiver)
        print("---------------")

        county_receiver = re.search(r'MUNICÍPIO\s+(FONE\/FAX\s+)?(.+)', text, re.M | re.I)
        county_receiver = str(county_receiver.group()).replace('MUNICÍPIO', '')
        county_receiver = county_receiver.replace('\n', '')
        county_receiver = county_receiver.replace('FONE/FAX', '')

        print("---------------")
        print(county_receiver)
        print("---------------")

        uf_receiver = re.search(
            r'DESTINATÁRIO[\W|\w]+UF\s+([a-zA-Z]{2})\s[\W|\w]+CÁLCULO DO IMPOSTO',
            text,
            re.M | re.I
        )
        uf_receiver = str(uf_receiver.group())
        uf_receiver = re.search(r'UF\s+([a-zA-Z]{2})', uf_receiver, re.M | re.I)
        uf_receiver = str(uf_receiver.group()).replace('\n', '')
        uf_receiver = uf_receiver.replace('\n', '')
        uf_receiver = uf_receiver.replace('UF', '')
        uf_receiver = uf_receiver[-2:]

        print("---------------")
        print(uf_receiver)
        print("---------------")

        re_p1 = r'DESTINATÁRIO[\W|\w]+FONE\/FAX[\W|\w]*\s(\(\d{2}\)\s?\d{5}\-\d{3}|'
        re_p2 = r'\(\d{2}\)\s?\d{5}\-\d{4}|\d{10})\s[\W|\w]+CÁLCULO DO IMPOSTO'
        phone_receiver = re.search(
            re_p1+re_p2,
            text,
            re.M | re.I
        )
        if phone_receiver:
            phone_receiver = str(phone_receiver.group()).replace('FONE/FAX', '')
            phone_receiver = re.search(
                r'(\(\d{2}\)\s?\d{5}\-\d{3}|\(\d{2}\)\s?\d{5}\-\d{4}|\d{10})',
                phone_receiver,
                re.M | re.I
            )
            phone_receiver = str(phone_receiver.group()).replace('\n', '')
            phone_receiver = phone_receiver.replace('(', '')
            phone_receiver = phone_receiver.replace(')', '')
            phone_receiver = phone_receiver.replace('-', '')
            phone_receiver = phone_receiver.replace(' ', '')
            phone_receiver = phone_receiver[-10:]
        else:
            phone_receiver = ''

        print("---------------")
        print(phone_receiver)
        print("---------------")

        receiver = Receiver.objects.create(
            cpf_cnpj=cpnj_cpf_receiver,
            name=name_receiver,
            address=address_receiver,
            neighborhood=neighborhood_receiver,
            cep=cep_receiver,
            county=county_receiver,
            uf=uf_receiver,
            phone=phone_receiver,
        )

    return receiver


def search_create_seller(cnpj_seller, text, uf_code_seller):

    if Seller.objects.filter(cnpj=cnpj_seller).count() == 1:
            seller = Seller.objects.get(cnpj=cnpj_seller)
    else:
        # Procurar os atributos do Seller

        name_seller = re.search(r'SÉRIE[0-9\:\s]+([a-zA-Z ]+)', text, re.M | re.I)
        name_seller = str(name_seller.group())
        name_seller = re.search(r'(\s[a-zA-Z ]+\s)', name_seller, re.M | re.I)
        name_seller = str(name_seller.group()).replace('\n', '')
        name_seller = name_seller.replace('SÉRIE', '')
        name_seller = name_seller.replace('LTDA', '')

        print("-------------------")
        print(name_seller)
        print("-------------------")

        cep_seller = re.search(
            r'SÉRIE[\W|\w]+\s(\d{5}\-\d{3}|\d{2}\.\d{3}\-\d{3}|\d{8})\s[\W|\w]+NATUREZA DA OPERAÇÃO',
            text,
            re.M | re.I
        )
        cep_seller = str(cep_seller.group())
        cep_seller = re.search(
            r'\s(\d{5}\-\d{3}|\d{2}\.\d{3}\-\d{3}|\d{8})\s',
            cep_seller,
            re.M | re.I
        )
        cep_seller = str(cep_seller.group()).replace('\n', '')
        cep_seller = cep_seller.replace('-', '')
        cep_seller = cep_seller.replace('.', '')
        cep_seller = cep_seller.replace('/', '')
        cep_seller = cep_seller.replace(' ', '')

        print("-------------------")
        print(cep_seller)
        print("-------------------")

        seller = Seller.objects.create(
            cnpj=cnpj_seller,
            name=name_seller,
            cep=cep_seller,
            uf_code=uf_code_seller,
        )

    return seller


class InvoiceView(APIView):

    parser_classes = (MultiPartParser,)

    def post(self, request, format=None):

        dict_invoice = {}

        pdf = request.FILES['file']

        dict_invoice['file'] = pdf

        pdfReader = PyPDF2.PdfFileReader(pdf)

        num_pages = pdfReader.numPages
        count = 0
        text1 = ""

        while count < num_pages:
            pageObj = pdfReader.getPage(count)
            count += 1
            text1 += pageObj.extractText()

        text = convert_pdf_to_txt(pdf)

        # VALIDAÇÃO-PDF/NF

        if text != "":
            text = text
        else:
            return Response(status=400)

        tokens = word_tokenize(text)

        punctuations = ['(', ')', ';', ':', '[', ']', ',']

        keywords = [word for word in tokens if not (word in punctuations)]

        validation_words = [
            "WWW.NFE.FAZENDA.GOV.BR/PORTAL",
            "www.nfe.fazenda.gov.br/portal",
            "WWW.NFE.FAZENDA.GOV.BR",
            "www.nfe.fazenda.gov.br",
        ]

        i = 0

        for word in validation_words:
            if word in keywords:
                i = i + 1

        if i < 1:
            return Response(status=400)

        dict_invoice['text'] = text

        # Parser AccessKey

        new_text = text.replace('\n', '')

        print(text)

        access_key = re.search(
            r'\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}\s+\d{4}',
            new_text,
            re.M | re.I
        )

        if not access_key:
            return Response({'error': 'Chave de acesso da nota fiscal não encontrada no pdf!'}, status=400)

        access_key = str(access_key.group()).replace(' ', '')

        uf_code_seller = access_key[0:2]

        cnpj_seller = access_key[6:20]

        number = access_key[25:34]

        dict_invoice['access_key'] = access_key
        dict_invoice['number'] = number

        print("-------------------")
        print(access_key)
        print("-------------------")

        # Parser CNPJ/CPF reveiver

        cpnj_cpf_receiver = re.findall(
            r'([\s+|\n]\d{11}\s+|[\s+|\n]\d{14}\s+|\d{3}\.\d{3}\.\d{3}\-\d{2}|\d{2}\.\d{3}\.\d{3}\/\d{4}\-\d{2})',
            text,
            re.M | re.I
        )

        if not cpnj_cpf_receiver or len(cpnj_cpf_receiver) < 2:
            return Response({'error': 'Cnpj ou cpf do destinatário da nota fiscal não encontrado no pdf!'}, status=400)

        for i in range(len(cpnj_cpf_receiver)):
            cpnj_cpf_receiver[i] = cpnj_cpf_receiver[i].replace('-', '')
            cpnj_cpf_receiver[i] = cpnj_cpf_receiver[i].replace('.', '')
            cpnj_cpf_receiver[i] = cpnj_cpf_receiver[i].replace('/', '')
            cpnj_cpf_receiver[i] = cpnj_cpf_receiver[i].replace('\n', '')
            cpnj_cpf_receiver[i] = cpnj_cpf_receiver[i].replace(' ', '')

        if cnpj_seller in cpnj_cpf_receiver:
            cpnj_cpf_receiver.remove(cnpj_seller)

        cpnj_cpf_receiver = cpnj_cpf_receiver[0]

        print("-------------------")
        print(cpnj_cpf_receiver)
        print("-------------------")

        # Verifica existencia de entidades

        receiver = search_create_receiver(cpnj_cpf_receiver, text)
        if not receiver:
            return Response(
                {'error': 'Atributos referentes ao destinatário da nota fiscal não encontrados no pdf!'},
                status=400
            )

        dict_invoice['receiver'] = receiver.pk

        seller = search_create_seller(cnpj_seller, text, uf_code_seller)
        if not seller:
            return Response(
                {'error': 'Atributos referentes ao emitente da nota fiscal não encontrados no pdf!'},
                status=400
            )

        dict_invoice['seller'] = seller.pk

        # NATUREZA DA OPERAÇÃO Parser

        operation_nature = re.search(
            r'NATUREZA DA OPERAÇÃO\s+(.+)',
            text,
            re.M | re.I
        )
        if not operation_nature:
            return Response(
                {'error': 'Naturaza da operação da nota fiscal não encontrada no pdf!'},
                status=400
            )
        operation_nature = str(operation_nature.group()).replace('NATUREZA DA OPERAÇÃO', '')
        operation_nature = operation_nature.replace('\n', '')

        print("-------------------")
        print(operation_nature)
        print("-------------------")

        dict_invoice['operation_nature'] = operation_nature

        # PROTOCOLO DE AUTORIZAÇÃO Parser

        authorization_protocol = re.search(
            r'(\d{15}(\s|\s\-\s)\d{2}\/\d{2}\/\d{4} \d{2}\:\d{2}(\:\d{2})?)',
            text,
            re.M | re.I
        )
        if not authorization_protocol:
            return Response(
                {'error': 'Protocolo de autorização da nota fiscal não encontrado no pdf!'},
                status=400
            )
        authorization_protocol = str(authorization_protocol.group())
        authorization_protocol = authorization_protocol.replace(' -', '')

        print("-------------------")
        print(authorization_protocol)
        print("-------------------")

        dict_invoice['authorization_protocol'] = authorization_protocol

        # INSCRIÇÃO ESTADUAL Parser

        state_registration = re.search(r'INSCRIÇÃO ESTADUAL\s+(\d+)\s', text, re.M | re.I)
        if not state_registration:
            return Response({'error': 'Inscrição estadual da nota fiscal não encontrada no pdf!'}, status=400)
        state_registration = str(state_registration.group())
        state_registration = state_registration.replace('INSCRIÇÃO ESTADUAL', '')
        state_registration = state_registration.replace(' ', '')
        state_registration = state_registration.replace('\n', '')

        print("-------------------")
        print(state_registration)
        print("-------------------")

        dict_invoice['state_registration'] = state_registration

        # VALOR TOTAL DOS PRODUTOS Parser

        values = re.findall(r'\s([\d+|\.]+\,\d{2})\s', text, re.M | re.I)

        re_nfv1 = r'(\d{7}\.\d{3}\s+(\d{2}\/\d{2}\/\d{4})'
        re_nfv2 = r'\s+([\d+|\.]+\,\d{2})\s)|(VALOR NOTA\s+((R\$\s)?[\d+|\.]+\,\d{2})\s)'
        total_invoice_value = re.search(
            re_nfv1+re_nfv2,
            text,
            re.M | re.I
        )
        if not total_invoice_value or not values:
            return Response({'error': 'Valor da nota fiscal não encontrada no pdf!'}, status=400)
        total_invoice_value = str(total_invoice_value.group())
        total_invoice_value = total_invoice_value.replace(' ', '')
        total_invoice_value = total_invoice_value.replace('/', '')
        total_invoice_value = total_invoice_value.replace('.', '')

        if "VALORNOTA" in total_invoice_value:
            total_invoice_value = total_invoice_value.replace('VALORNOTA', '')
            total_invoice_value = total_invoice_value.replace('R$', '')
        else:
            total_invoice_value = total_invoice_value[18:]

        total_invoice_value = total_invoice_value.replace('\n', '.')
        total_invoice_value = total_invoice_value.replace(',', '.')

        if total_invoice_value[0] == '.':
            total_invoice_value = total_invoice_value[1:]
        if total_invoice_value[-1] == '.':
            total_invoice_value = total_invoice_value[:-1]

        print("-------------------")
        print(total_invoice_value)
        print("-------------------")

        dict_invoice['total_invoice_value'] = total_invoice_value

        i = 0

        print("-------------------")
        print(values)
        print("-------------------")

        for value in values:
            values[i] = value.replace('.', '')
            values[i] = values[i].replace(',', '.')
            values[i] = float(values[i])
            i += 1

        bigger = 0.0

        for value in values:
            if value > bigger:
                bigger = value

        # DATAS E HORAS Parser

        dates = re.findall(r'(\d{2}\/\d{2}\/\d{4})', text1, re.M | re.I)
        hours = re.findall(r'(\d{2}\:\d{2}(\:\d{2})?)', text, re.M | re.I)
        if not dates or not hours:
            return Response({'error': 'Datas não encontradas no pdf!'}, status=400)

        print("-------------------")
        print(dates)
        print(hours)
        print("-------------------")

        emission_date = dates[1]
        entry_exit_datetime = str(dates[2] + ' ' + hours[1][0])

        print("-------------------")
        print(emission_date)
        print(entry_exit_datetime)
        print("-------------------")

        emission_date = emission_date.split('/')
        entry_exit_datetime = entry_exit_datetime.split(' ')
        time = entry_exit_datetime[1]
        entry_exit_datetime = entry_exit_datetime[0].split('/')

        emission_date = emission_date[2] + '-' + emission_date[1] + '-' + emission_date[0]
        x = entry_exit_datetime[2] + '-' + entry_exit_datetime[1] + '-' + entry_exit_datetime[0] + ' ' + time
        entry_exit_datetime = x

        dict_invoice['emission_date'] = emission_date
        dict_invoice['entry_exit_datetime'] = entry_exit_datetime

        serializer = InvoiceSerializer(data=dict_invoice)

        print(dict_invoice)

        if serializer.is_valid():
            serializer.save()
            return Response(serializer.data, status=200)
        else:
            return Response(serializer.errors, status=400)
