#include "Parser.h"
#include "Utils.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QTextStream>

#include <QDebug>

Parser::Parser()
{
    _fileName = "";

    _search = new Search();
}

///
/// \brief TrySimulateRectHeader
/// \param headerRect
/// \param possibleValues
/// \param maxPageHeight
/// \param maxPageWidth
/// \return
///
QRect Parser::TrySimulateRectHeader(QRect headerRect, QList<sTEXTDATA*>* possibleValues, int maxPageHeight, int maxPageWidth)
{
    QList<sTEXTDATA*> inRange;
    sTEXTDATA* tmpData = nullptr;

    // Keep headerRect without edits.
    const QRect originalHeaderRect = headerRect;

    // Variables used for calculation of distance.
    int distance = 0;
    int distanceClose = originalHeaderRect.height() / 2;

    // When only one neighbor was found in the same column
    // then there is only one possibility: left or right side.
    bool oneColumn = false;

    // Get all the values that are close to the header in Y.
    for(auto it = possibleValues->begin(); it != possibleValues->end(); ++it)
    {
        tmpData = (*it);

        // Check if is in same position, if yes, let ignore, because is our current header.
        if(tmpData->top == originalHeaderRect.y() &&
           tmpData->left == originalHeaderRect.x())
            continue;

        // Calcule distance between header and tmpData.
        distance = (tmpData->top > originalHeaderRect.y()) ?
                    (tmpData->top - originalHeaderRect.y()) :
                    (originalHeaderRect.y() - tmpData->top);

        // Check if distance calculed is less than lastest distance close.
        if(distance <= distanceClose)
        {
            inRange.push_back(tmpData);
        }
    }

    // Check if not found any value.
    if(inRange.size() == 0)
    {
        qDebug() << "This shouldn't to happen.";
        return headerRect;
    }

    // Let to process values.
    sTEXTDATA* leftData = nullptr;
    sTEXTDATA* rightData = nullptr;

    // Check if found only 1 value in current row
    // Then check if value is in right or left side of our header data.
    if(inRange.size() == 1)
    {
        // Get first value from list.
        tmpData = inRange.first();

        // Set oneColumn in true, because don't is more necessary to search new values.
        oneColumn = true;

        // Calcule max width value, if value is greater than left variable
        // Then is data in right side, else is left side.
        int sizeTotal = originalHeaderRect.left() + originalHeaderRect.width();
        if(tmpData->left > sizeTotal)
        {
            rightData = tmpData;
        }
        else
        {
            leftData = tmpData;
        }
    }

    // Optimize list, so you will not have to go through the entire list again
    // but only for possible values.
    QList<sTEXTDATA*> tmpPossibleValues;
    bool sameInRange = false;

    for(auto it = possibleValues->begin(); it != possibleValues->end(); ++it)
    {
        tmpData = (*it);

        if(tmpData->top < originalHeaderRect.top() ||
           tmpData->top > ((originalHeaderRect.top() + originalHeaderRect.height()) + (SPACE_HEIGHT_BETWEEN_HEADER_VALUE * 2)))
            continue;

        sameInRange = false;
        for(auto rangeIt = inRange.begin(); rangeIt != inRange.end(); ++rangeIt)
        {
            if((*rangeIt)->left == tmpData->left &&
               (*rangeIt)->top == tmpData->top)
            {
                sameInRange = true;
                break;
            }
        }

        if(sameInRange)
            continue;

        tmpPossibleValues.push_back(tmpData);
    }

    if(tmpPossibleValues.length() > 0)
    {
        possibleValues = &tmpPossibleValues;
    }

    distanceClose = 0;
    distance = originalHeaderRect.left();

    /********************************************************/
    // 1. Get left side more close.
    if(oneColumn == false)
    {
        for(auto it = inRange.begin(); it != inRange.end(); ++it)
        {
            tmpData = (*it);

            if(tmpData->left > originalHeaderRect.left())
                continue;

            distanceClose = originalHeaderRect.left() - tmpData->left;
            if(distance > distanceClose)
            {
                distance = distanceClose;
                leftData = tmpData;
            }
        }
    }

    // Check if we got left data for be processed.
    if(leftData != nullptr)
    {
        QList<sTEXTDATA*> values;

        QRect leftRect(QPoint(leftData->left, leftData->top),
                       QSize(leftData->width, leftData->height));

        int diffLeft = (originalHeaderRect.left() - leftRect.left()) - 1;

        // Update rect of the right.

        // TODO : find a better way to increase height than to use defined value 10.
        QRect newLeftRect(QPoint(leftData->left, leftData->top),
                          QSize(diffLeft, leftData->height + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

        QRect tmpRect;

        for(auto it = possibleValues->begin(); it != possibleValues->end(); ++it)
        {
            tmpData = (*it);

            // Check if is in same position, if yes, let ignore, because is our current header.
            if(tmpData->top == leftRect.y() &&
               tmpData->left == leftRect.x())
                continue;

            // Sometimes the position of the left of the value is smaller than that of the header
            // so we have to ignore it, otherwise the algorithm will think that this value belongs
            // to another header.
            if((tmpData->left + tmpData->width) > originalHeaderRect.left())
                continue;

            tmpRect = QRect(QPoint(tmpData->left, tmpData->top),
                            QSize(tmpData->width, tmpData->height));

            // Check if current data intersects with our header rect.
            if(newLeftRect.intersects(tmpRect) &&
              (tmpData->left + tmpData->width) > (leftData->left + leftData->width))
            {
                values.push_back(tmpData);
            }
        }

        // Check if got some value for be processed.
        if(values.size() == 0)
        {
            // There is no value below the right header.
            // So only rightData coordinates will be used to update.
            int newLeft = (leftRect.left() + leftRect.width()) + 1;
            int newSizeWidth = (originalHeaderRect.left() - newLeft) + originalHeaderRect.width();

            headerRect = QRect(QPoint(newLeft, originalHeaderRect.top()),
                               QSize(newSizeWidth, originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

            rightData = nullptr;
        }
        else if(values.size() == 1)
        {
            tmpData = values.first();

            // Calcule maximium left and check if this overflow with headerRect left.
            int newLeft = (tmpData->left + tmpData->width) + 1;

            int newSizeWidth = (originalHeaderRect.left() - newLeft) + originalHeaderRect.width();
            headerRect = QRect(QPoint(newLeft, originalHeaderRect.top()),
                               QSize(newSizeWidth, originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

            leftData = tmpData;
        }
        else
        {
            distance = 0;
            distanceClose = 0;

            // Reset tmpData.
            tmpData = nullptr;

            // In this case, we got more than 1 values closers.
            for(auto it = values.begin(); it != values.end(); ++it)
            {
                rightData = (*it);

                distanceClose = originalHeaderRect.left() - rightData->left;
                if(distanceClose > distance)
                {
                    distance = distanceClose;
                    tmpData = rightData;
                }
            }

            if(tmpData == nullptr)
            {
                // TODO : Necessary to check some case than happen here.
                qDebug() << "This shouldn't to happen!";
                return headerRect;
            }
            else
            {
                // Calcule maximium left and check if this overflow with headerRect left.
                int newLeft = (tmpData->left + tmpData->width) + 1;

                int newSizeWidth = (originalHeaderRect.left() - newLeft) + originalHeaderRect.width();
                headerRect = QRect(QPoint(newLeft, originalHeaderRect.top()),
                                   QSize(newSizeWidth, originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

                leftData = tmpData;
            }
        }
    }

    // Update distance.
    distance = maxPageWidth;
    distanceClose = 0;

    bool isLastColumn = true;

    /********************************************************/
    // 2. Get right side more close.
    if(oneColumn == false)
    {
        int totalWidth = originalHeaderRect.left() + originalHeaderRect.width();
        for(auto it = inRange.begin(); it != inRange.end(); ++it)
        {
            tmpData = (*it);

            if(isLastColumn &&
               (tmpData->left + tmpData->width) > totalWidth)
            {
                isLastColumn = false;
            }

            if(tmpData->left < totalWidth)
                continue;

            distanceClose = tmpData->left - totalWidth;
            if(distanceClose < distance)
            {
                distance = distanceClose;
                rightData = tmpData;
            }
        }
    }

    // Check if we found some value in right side.
    if(rightData == nullptr && isLastColumn)
    {
        // There is no value below the right header.
        // So only rightData coordinates will be used to update.
        int newLeft = (originalHeaderRect.left() != headerRect.left()) ?
                       headerRect.left() :
                       originalHeaderRect.left();

        int newSizeWidth = (originalHeaderRect.width() != headerRect.width()) ?
                    headerRect.width() + (maxPageWidth - (originalHeaderRect.left() + originalHeaderRect.width())) - 1 :
                    originalHeaderRect.width() + (maxPageWidth - (originalHeaderRect.left() + originalHeaderRect.width())) - 1;

        headerRect = QRect(QPoint(newLeft, originalHeaderRect.top()),
                           QSize(newSizeWidth, originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));
    }
    else if(rightData != nullptr)
    {
        // Temporary variable used for intersects.
        QRect tmpRect;

        QList<sTEXTDATA*> values;

        QRect rightRect(QPoint(rightData->left, rightData->top),
                        QSize(rightData->width, rightData->height));

        QRect testQuantityRect(QPoint(originalHeaderRect.left(), originalHeaderRect.top()),
                               QSize(originalHeaderRect.width() + ((rightData->left - (originalHeaderRect.left() + originalHeaderRect.width())) - 1),
                                     originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

        for(auto it = possibleValues->begin(); it != possibleValues->end(); ++it)
        {
            tmpData = (*it);

            // Check if is in same position, if yes, let ignore, because is our current header.
            if((tmpData->top == rightRect.y() && tmpData->left == rightRect.x()) ||
               (tmpData->top == originalHeaderRect.top() && tmpData->left == originalHeaderRect.left()))
                continue;

            tmpRect = QRect(QPoint(tmpData->left, tmpData->top),
                            QSize(tmpData->width, tmpData->height));

            if(testQuantityRect.intersects(tmpRect))
            {
                values.push_back(tmpData);
            }
        }

        if(values.length() == 1)
        {
            values.clear();
        }
        else
        {
            values.clear();

            int diffLeft = (rightData->left - (originalHeaderRect.left() + originalHeaderRect.width())) - 1;
            int newWidth = (originalHeaderRect.left() + originalHeaderRect.width()) + 1;

            // Update rect of the right.

            // TODO : find a better way to increase height than to use defined value 10.
            QRect newRightRect(QPoint(newWidth, rightData->top),
                               QSize(diffLeft, rightData->height + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

            // TODO : find a better way to increase height than to use defined value 10.
            QRect tmpRectHeader(QPoint(headerRect.left(), headerRect.top()),
                                QSize(headerRect.width(), headerRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

            for(auto it = possibleValues->begin(); it != possibleValues->end(); ++it)
            {
                tmpData = (*it);

                // Check if is in same position, if yes, let ignore, because is our current header.
                if(tmpData->top == rightRect.y() &&
                   tmpData->left == rightRect.x())
                    continue;

                tmpRect = QRect(QPoint(tmpData->left, tmpData->top),
                                QSize(tmpData->width, tmpData->height));

                // Check if current data intersects with our header rect.
                if(newRightRect.intersects(tmpRect))
                {
                    // Check if it intercepts with the original header
                    // It's own value we're looking for, but we'll ignore it for now.
                    if(tmpRectHeader.intersects(tmpRect))
                        continue;

                    values.push_back(tmpData);
                }
            }
        }

        // -----------------------------------
        if(values.size() == 0)
        {
            // There is no value below the right header.
            // So only rightData coordinates will be used to update.
            int newLeft = (originalHeaderRect.left() != headerRect.left()) ?
                           headerRect.left() :
                           originalHeaderRect.left();

            int newSizeWidth = (originalHeaderRect.width() != headerRect.width()) ?
                        headerRect.width() + (rightData->left - (originalHeaderRect.left() + originalHeaderRect.width())) - 1 :
                        originalHeaderRect.width() + (rightData->left - (originalHeaderRect.left() + originalHeaderRect.width())) - 1;

            headerRect = QRect(QPoint(newLeft, originalHeaderRect.top()),
                               QSize(newSizeWidth, originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

            rightData = nullptr;
        }
        else if(values.size() == 1)
        {
            tmpData = values.first();

            // There is no value below the right header.
            // So only rightData coordinates will be used to update.
            int newLeft = (originalHeaderRect.left() != headerRect.left()) ?
                           headerRect.left() :
                           originalHeaderRect.left();

            int newSizeWidth = (originalHeaderRect.width() != headerRect.width()) ?
                        headerRect.width() + (tmpData->left - (originalHeaderRect.left() + originalHeaderRect.width())) - 1 :
                        originalHeaderRect.width() + (tmpData->left - (originalHeaderRect.left() + originalHeaderRect.width())) - 1;

            headerRect = QRect(QPoint(newLeft, originalHeaderRect.top()),
                               QSize(newSizeWidth, originalHeaderRect.height() + SPACE_HEIGHT_BETWEEN_HEADER_VALUE));

            rightData = tmpData;
        }
        else
        {
            // TODO : Necessary to check some case than can happen here.
            qDebug() << "This shouldn't to happen!";
            return headerRect;
        }
    }

    return headerRect;
}

///
/// \brief TryGetValue
/// \param header
/// \param possibleValues
/// \param value
/// \param maxPageHeight
/// \param maxPageWidth
/// \return
///
bool Parser::TryGetValue(sTEXTDATA* header, QList<sTEXTDATA*>* possibleValues, QString* value, int maxPageHeight, int maxPageWidth)
{
    QList<sTEXTDATA*> values;
    sTEXTDATA* tmpData = nullptr;

    QRect headerRect(QPoint(header->left, header->top),
                     QSize(header->width, header->height));

    headerRect = TrySimulateRectHeader(headerRect, possibleValues, maxPageHeight, maxPageWidth);

    QRect tmpRect;

    for(auto it = possibleValues->begin(); it != possibleValues->end(); ++it)
    {
        tmpData = (*it);

        // Ignore if is same header.
        if(tmpData == header || tmpData->text == header->text)
            continue;

        tmpRect = QRect(QPoint(tmpData->left, tmpData->top), QSize(tmpData->width, tmpData->height));

        if(headerRect.intersects(tmpRect))
        {
            values.push_back(tmpData);
        }
    }

    if(values.size() == 0)
    {
        *value = "";
        return true;
    }
    else if(values.size() == 1)
    {
        tmpData = values.first();
        *value = tmpData->text;

        return true;
    }
    else
    {
        sTEXTDATA* txtData = nullptr;
        int distanceTop = header->height / 2;

        int distance = -1;
        int tmpDistance = 0;

        for(auto it = values.begin(); it != values.end(); ++it)
        {
            tmpData = (*it);

            if(tmpData->top <= (header->top + distanceTop))
                continue;

            tmpDistance = (tmpData->left > header->left) ?
                        tmpData->left - header->left :
                        header->left - tmpData->left;

            if(distance == -1)
            {
                distance = tmpDistance;
                txtData = tmpData;
            }
            else if(tmpDistance < distance)
            {
                distance = tmpDistance;
                txtData = tmpData;
            }
        }

        if(txtData == nullptr)
        {
            *value = "";
            return false;
        }
        else
        {
            *value = txtData->text;
            return true;
        }
    }
}

bool Parser::FindValueData(QString value, QList<sTEXTDATA*> list, QRect* rect)
{
    sTEXTDATA* data;

    QList<sTEXTDATA*> find;

    for(auto it = list.begin(); it != list.end(); ++it)
    {
        data = (*it);
        if(data != nullptr && data->text == value)
        {
            find.push_back(data);
        }
    }

    if(find.length() == 0)
    {
        *rect = QRect(QPoint(-1, -1), QSize(-1, -1));
        return false;
    }
    else if(find.length() == 1)
    {
        data = find.first();

        *rect = QRect(QPoint(data->left, data->top), QSize(data->width, data->height));
        return true;
    }
    else
    {
        // TODO:
    }

    return false;
}

QString Parser::ConvertEnumToText(eINVOICE_HEADER header, int value)
{
    switch(header)
    {
    case MAIN:
    {
        switch(value)
        {
            case H_ACCESS_KEY:
            {
                return "CHAVE DE ACESSO";
            }
            case H_NATURE_OPERATION:
            {
                return "NATUREZA DA OPERAÇÃO";
            }
            case H_PROTOCOL_AUTHORIZATION_USE:
            {
                return "PROTOCOLO DE AUTORIZAÇÃO DE USO";
            }
            case H_STATE_REGISTRATION:
            {
                return "INSCRIÇÃO ESTADUAL";
            }
            case H_STATE_REGISTRATION_SUB_TAXATION:
            {
                return "INSCRIÇÃO ESTADUAL SUBSTITUIÇÃO TRIBUTÁRIO";
            }
            case H_CNPJ:
            {
                return "CNPJ";
            }
        }
    }
    break;
    case ADDRESSEE_SENDER:
    {
        switch(value)
        {
        case A_S_NAME_SOCIAL_REASON:
        {
            return "NOME/RAZÃO SOCIAL";
        }
        case A_S_CPNJ_CPF:
        {
            return "CNPJ/CPF";
        }
        case A_S_EMISSION_DATE:
        {
            return "DATA DA EMISSÃO";
        }
        case A_S_ADDRESS:
        {
            return "ENDEREÇO";
        }
        case A_S_NEIGHBORHOOD_DISTRICT:
        {
            return "BAIRRO/DISTRITO";
        }
        case A_S_CEP:
        {
            return "CEP";
        }
        case A_S_OUTPUT_INPUT_DATE:
        {
            return "DATA DE SAÍDA/ENTRADA";
        }
        case A_S_COUNTY:
        {
            return "MUNICÍPIO";
        }
        case A_S_PHONE_FAX:
        {
            return "TELEFONE/FAX";
        }
        case A_S_UF:
        {
            return "UF";
        }
        case A_S_STATE_REGISTRATION:
        {
            return "INSCRIÇÃO ESTADUAL";
        }
        case A_S_EXIT_TIME:
        {
            return "HORA DE SAÍDA";
        }
        default:
        {
            return "DESTINATÁRIO/REMETENTE";
        }
        }
    }
    //break;
    case TAX_CALCULATION:
    {
        switch(value)
        {
        case T_C_ICMS_CALCULATION_BASIS:
        {
            return "BASE DE CÁLCULO DE ICMS";
        }
        case T_C_COST_ICMS:
        {
            return "VALOR DO ICMS";
        }
        case T_C_CALCULATION_BASIS_ICMS_ST:
        {
            return "BASE DE CÁLCULO ICMS SUBSTITUIÇÃO";
        }
        case T_C_VALUE_ICMS_REPLACEMENT:
        {
            return "VALOR DO ICMS SUBSTITUIÇÃO";
        }
        case T_C_TOTAL_VALUE_PRODUCTS:
        {
            return "VALOR TOTAL DOS PRODUTOS";
        }
        case T_C_COST_FREIGHT:
        {
            return "VALOR DO FRETE";
        }
        case T_C_COST_INSURANCE:
        {
            return "VALOR DO SEGURO";
        }
        case T_C_DISCOUNT:
        {
            return "DESCONTO";
        }
        case T_C_OTHER_EXPENDITURE:
        {
            return "OUTRAS DESPESAS ACESSÓRIAS";
        }
        case T_C_COST_IPI:
        {
            return "VALOR DO IPI";
        }
        case T_C_APPROXIMATE_COST_TAXES:
        {
            return "VALOR APROXIMADO DOS TRIBUTOS";
        }
        case T_C_COST_TOTAL_NOTE:
        {
            return "VALOR TOTAL DA NOTA";
        }
        default:
        {
            return "CÁLCULO DO IMPOSTO";
        }
        }
    }
    //break;
    case CONVEYOR_VOLUMES:
    {
        switch(value)
        {
        case C_V_SOCIAL_REASON:
        {
            return "RAZÃO SOCIAL";
        }
        case C_V_FREIGHT_ACCOUNT:
        {
            return "FRETE POR CONTA";
        }
        case C_V_CODE_ANTT:
        {
            return "CÓDIGO ANTT";
        }
        case C_V_VEHICLE_PLATE:
        {
            return "PLACA DO VEÍCULO";
        }
        case C_V_UF_1:
        {
            return "UF";
        }
        case C_V_CNPJ_CPF:
        {
            return "CNPJ/CPF";
        }
        case C_V_ADDRESS:
        {
            return "ENDEREÇO";
        }
        case C_V_COUNTY:
        {
            return "MUNICÍPIO";
        }
        case C_V_UF_2:
        {
            return "UF";
        }
        case C_V_STATE_REGISTRATION:
        {
            return "INSCRIÇÃO ESTADUAL";
        }
        case C_V_QUANTITY:
        {
            return "QUANTIDADE";
        }
        case C_V_SPECIES:
        {
            return "ESPÉCIE";
        }
        case C_V_MARK:
        {
            return "MARCA";
        }
        case C_V_NUMBERING:
        {
            return "NUMERAÇÃO";
        }
        case C_V_GROSS_WEIGHT:
        {
            return "PESO BRUTO";
        }
        case C_V_NET_WEIGHT:
        {
            return "PESO LIQUIDO";
        }
        default:
        {
            return "TRANSPORTADOR/VOLUMES TRANSPORTADOS";
        }
        }
    }
    //break;
    case PRODUCT_SERVICE_DATA:
    {
        return "DADOS DO PRODUTO/SERVIÇO";
    }
    //break;
    case ISSQN_CALCULATION:
    {
        switch(value)
        {
        case I_C_MUNICIPAL_REGISTRATION:
        {
            return "INSCRIÇÃO MUNICIPAL";
        }
        case I_C_TOTAL_COST_SERVICES:
        {
            return "VALOR TOTAL DOS SERVIÇOS";
        }
        case I_C_ISSQN_CALCULATION_BASE:
        {
            return "BASE DE CALCULO DO ISSQN";
        }
        case I_C_COST_ISSQN:
        {
            return "VALOR DO ISSQN";
        }
        default:
        {
            return "CÁLCULO DO ISSQN";
        }
        }
    }
    //break;
    case ADDITIONAL_DATA:
    {
        return "DADOS ADICIONAIS";
    }
    }

    return "";
}

sINVOICEHEADER* Parser::GetInvoiceHeader(QList<sINVOICEHEADER*> list, int value)
{
    for(auto it = list.begin(); it != list.end(); ++it)
    {
        if((*it) && (*it)->header == value)
            return (*it);
    }

    return nullptr;
}

QList<sINVOICEHEADER*> Parser::ProcessInvoiceHeader(QList<sTEXTDATA*> possibleHeaders, int maxWidth, int maxHeight)
{
    QList<sINVOICEHEADER*> invoiceHeaderList;

    sINVOICEHEADER* invoiceHeader = nullptr;
    sINVOICEHEADER* tmpHeader = nullptr;

    QRect tmpRect;
    QString tmpStr;
    QString tmpHeaderStr;
    //QList<QString> tmpStrList;

    sTEXTDATA* tmpTextData = nullptr;

    // Process header rects.
    for(int header = MAX - 1; header >= MAIN; header--)
    {
        tmpHeaderStr = "";
        tmpTextData = nullptr;
        //tmpStrList.clear();

        switch(header)
        {
            case ADDITIONAL_DATA:
            {
                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = ADDITIONAL_DATA;

                tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                if(tmpTextData != nullptr)
                {
                    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                                    QSize(maxWidth, maxHeight - tmpRect.top()));

                    invoiceHeader->rect = tmpRect;
                    invoiceHeaderList.push_back(invoiceHeader);
                }

                //tmpStrList = ConvertEnumToText(invoiceHeader->header);
                //
                //for(auto it = tmpStrList.begin(); it != tmpStrList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    if(tmpStr.isEmpty())
                //        continue;
                //
                //    if(FindValueData(tmpStr, possibleHeaders, &tmpRect))
                //    {
                //        tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                        QSize(maxWidth, maxHeight - tmpRect.top()));
                //
                //        invoiceHeader->rect = tmpRect;
                //
                //        invoiceHeaderList.push_back(invoiceHeader);
                //        break;
                //    }
                //}
            }
            break;
            case ISSQN_CALCULATION:
            {
                tmpHeader = GetInvoiceHeader(invoiceHeaderList, ADDITIONAL_DATA);
                if(tmpHeader == nullptr)
                    continue;

                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = ISSQN_CALCULATION;

                tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                if(tmpTextData != nullptr)
                {
                    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                                    QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));

                    invoiceHeader->rect = tmpRect;
                    invoiceHeaderList.push_back(invoiceHeader);
                }

                //tmpStrList = ConvertEnumToText(invoiceHeader->header);
                //
                //for(auto it = tmpStrList.begin(); it != tmpStrList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    if(tmpStr.isEmpty())
                //        continue;
                //
                //    if(FindValueData(tmpStr, possibleHeaders, &tmpRect))
                //    {
                //        tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                        QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));
                //
                //        invoiceHeader->rect = tmpRect;
                //
                //        invoiceHeaderList.push_back(invoiceHeader);
                //        break;
                //    }
                //}
            }
            break;
            case PRODUCT_SERVICE_DATA:
            {
                tmpHeader = GetInvoiceHeader(invoiceHeaderList, ISSQN_CALCULATION);
                if(tmpHeader == nullptr)
                    continue;

                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = PRODUCT_SERVICE_DATA;

                tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                if(tmpTextData != nullptr)
                {
                    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                                    QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));

                    invoiceHeader->rect = tmpRect;
                    invoiceHeaderList.push_back(invoiceHeader);
                }

                //tmpStrList = ConvertEnumToText(invoiceHeader->header);
                //
                //for(auto it = tmpStrList.begin(); it != tmpStrList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    if(tmpStr.isEmpty())
                //        continue;
                //
                //    if(FindValueData(tmpStr, possibleHeaders, &tmpRect))
                //    {
                //        tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                        QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));
                //
                //        invoiceHeader->rect = tmpRect;
                //
                //        invoiceHeaderList.push_back(invoiceHeader);
                //        break;
                //    }
                //}
            }
            break;
            case CONVEYOR_VOLUMES:
            {
                tmpHeader = GetInvoiceHeader(invoiceHeaderList, PRODUCT_SERVICE_DATA);
                if(tmpHeader == nullptr)
                    continue;

                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = CONVEYOR_VOLUMES;

                tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                if(tmpTextData != nullptr)
                {
                    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                                    QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));

                    invoiceHeader->rect = tmpRect;
                    invoiceHeaderList.push_back(invoiceHeader);
                }

                //tmpStrList = ConvertEnumToText(invoiceHeader->header);
                //
                //for(auto it = tmpStrList.begin(); it != tmpStrList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    if(tmpStr.isEmpty())
                //        continue;
                //
                //    if(FindValueData(tmpStr, possibleHeaders, &tmpRect))
                //    {
                //        tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                        QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));
                //
                //        invoiceHeader->rect = tmpRect;
                //
                //        invoiceHeaderList.push_back(invoiceHeader);
                //        break;
                //    }
                //}
            }
            break;
            case TAX_CALCULATION:
            {
                tmpHeader = GetInvoiceHeader(invoiceHeaderList, CONVEYOR_VOLUMES);
                if(tmpHeader == nullptr)
                    continue;

                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = TAX_CALCULATION;

                tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                if(tmpTextData != nullptr)
                {
                    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                                    QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));

                    invoiceHeader->rect = tmpRect;
                    invoiceHeaderList.push_back(invoiceHeader);
                }

                //tmpStrList = ConvertEnumToText(invoiceHeader->header);
                //
                //for(auto it = tmpStrList.begin(); it != tmpStrList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    if(tmpStr.isEmpty())
                //        continue;
                //
                //    if(FindValueData(tmpStr, possibleHeaders, &tmpRect))
                //    {
                //        tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                        QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));
                //
                //        invoiceHeader->rect = tmpRect;
                //
                //        invoiceHeaderList.push_back(invoiceHeader);
                //        break;
                //    }
                //}
            }
            break;
            case ADDRESSEE_SENDER:
            {
                tmpHeader = GetInvoiceHeader(invoiceHeaderList, TAX_CALCULATION);
                if(tmpHeader == nullptr)
                    continue;

                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = ADDRESSEE_SENDER;

                tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                if(tmpTextData != nullptr)
                {
                    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                                    QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));

                    invoiceHeader->rect = tmpRect;
                    invoiceHeaderList.push_back(invoiceHeader);
                }

                //tmpStrList = ConvertEnumToText(invoiceHeader->header);
                //
                //for(auto it = tmpStrList.begin(); it != tmpStrList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    if(tmpStr.isEmpty())
                //        continue;
                //
                //    if(FindValueData(tmpStr, possibleHeaders, &tmpRect))
                //    {
                //        tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                        QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));
                //
                //        invoiceHeader->rect = tmpRect;
                //
                //        invoiceHeaderList.push_back(invoiceHeader);
                //        break;
                //    }
                //}
            }
            break;
            case MAIN:
            {
                tmpHeader = GetInvoiceHeader(invoiceHeaderList, ADDRESSEE_SENDER);
                if(tmpHeader == nullptr)
                    continue;

                invoiceHeader = new sINVOICEHEADER();
                invoiceHeader->header = MAIN;

                tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, 0 /*tmpRect.top()*/),
                                QSize(maxWidth, (tmpHeader->rect.top() - 1)/* - tmpRect.top()*/));

                invoiceHeader->rect = tmpRect;
                invoiceHeaderList.push_back(invoiceHeader);
            }
            break;
        }
    }

    return invoiceHeaderList;
}

sTEXTDATA* Parser::GetTextData(QString header, QList<sTEXTDATA*> possibleValues)
{
    sTEXTDATA* tmpData = nullptr;
    for(auto it = possibleValues.begin(); it != possibleValues.end(); ++it)
    {
        tmpData = (*it);
        if(tmpData != nullptr)
        {
            if(tmpData->text == header)
            {
                return tmpData;
            }
        }
    }

    return nullptr;
}

int Parser::GetMaxDataHeader(int header)
{
    switch(header)
    {
        case MAIN: return H_MAX;
        case ADDRESSEE_SENDER: return A_S_MAX;
        case TAX_CALCULATION: return T_C_MAX;
        case CONVEYOR_VOLUMES: return C_V_MAX;
        case PRODUCT_SERVICE_DATA: return 0;
        case ISSQN_CALCULATION: return I_C_MAX;
        default: return 0;
    }
}

void Parser::DebugShow()
{
    QString tmpStr;
    sINVOICEDATA* tmpInvoice = nullptr;

    for(auto it = _invoicesMap.begin(); it != _invoicesMap.end(); ++it)
    {
        switch(it.key())
        {
            case MAIN: tmpStr = "NF-E"; break;
            case ADDRESSEE_SENDER: tmpStr = "DESTINATÁRIO/REMETENTE"; break;
            case TAX_CALCULATION: tmpStr = "CÁLCULO DO IMPOSTO"; break;
            case CONVEYOR_VOLUMES: tmpStr = "TRANSPORTADOR/VOLUMES TRANSPORTADOS"; break;
            case PRODUCT_SERVICE_DATA: tmpStr = "DADOS DO PRODUTO/SERVIÇO"; break;
            case ISSQN_CALCULATION: tmpStr = "CÁLCULO DO ISSQN"; break;
        }

        printf("=============================================================================\n");
        printf("%s\n", tmpStr.toStdString().c_str());
        printf("=============================================================================\n");

        for(auto it2 = (*it).begin(); it2 != (*it).end(); ++it2)
        {
            tmpInvoice = (*it2);
            printf("[%s]\n", tmpInvoice->header.toStdString().c_str());
            printf("%s\n", tmpInvoice->value.toStdString().c_str());
        }

        printf("\n");
    }
}

QString Parser::ConvertToJsonHeader(int header, int value)
{
    QString tmpStr;

    switch(header)
    {
    case MAIN:
    {
        switch(value)
        {
            case H_ACCESS_KEY: return "main_access_key";
            case H_NATURE_OPERATION: return "main_nature_operation";
            case H_PROTOCOL_AUTHORIZATION_USE: return "main_protocol_authorization_use";
            case H_STATE_REGISTRATION: return "main_state_registration";
            case H_STATE_REGISTRATION_SUB_TAXATION: return "main_state_registration_sub_tax";
            case H_CNPJ: return "main_cnpj";
        }
    }
    break;
    case ADDRESSEE_SENDER:
    {
        switch(value)
        {
        case A_S_NAME_SOCIAL_REASON: return "sender_name_social";
        case A_S_CPNJ_CPF: return "sender_cnpj_cpf";
        case A_S_EMISSION_DATE: return "sender_emission_date";
        case A_S_ADDRESS: return "sender_address";
        case A_S_NEIGHBORHOOD_DISTRICT: return "sender_neighborhood_district";
        case A_S_CEP: return "sender_cep";
        case A_S_OUTPUT_INPUT_DATE: return "sender_out_input_date";
        case A_S_COUNTY: return "sender_county";
        case A_S_PHONE_FAX: return "sender_phone_fax";
        case A_S_UF: return "sender_uf";
        case A_S_STATE_REGISTRATION: return "sender_state_registration";
        case A_S_EXIT_TIME: return "sender_output_time";
        }
    }
    break;
    case TAX_CALCULATION:
    {
        switch(value)
        {
        case T_C_ICMS_CALCULATION_BASIS: return "tax_icms_basis";
        case T_C_COST_ICMS: return "tax_cost_icms";
        case T_C_CALCULATION_BASIS_ICMS_ST: return "tax_icms_basis_st";
        case T_C_VALUE_ICMS_REPLACEMENT: return "tax_cost_icms_replacement";
        case T_C_TOTAL_VALUE_PRODUCTS: return "tax_total_cost_products";
        case T_C_COST_FREIGHT: return "tax_cost_freight";
        case T_C_COST_INSURANCE: return "tax_cost_insurance";
        case T_C_DISCOUNT: return "tax_discount";
        case T_C_OTHER_EXPENDITURE: return "tax_other_expenditure";
        case T_C_COST_IPI: return "tax_cost_ipi";
        case T_C_APPROXIMATE_COST_TAXES: return "tax_approx_cost_taxes";
        case T_C_COST_TOTAL_NOTE: return "tax_cost_total_note";
        }
    }
    break;
    case CONVEYOR_VOLUMES:
    {
        switch(value)
        {
        case C_V_SOCIAL_REASON: return "conveyor_social_reason";
        case C_V_FREIGHT_ACCOUNT: return "conveyor_freight_account";
        case C_V_CODE_ANTT: return "conveyor_code_antt";
        case C_V_VEHICLE_PLATE: return "conveyor_vehicle_plate";
        case C_V_UF_1: return "conveyor_uf_1";
        case C_V_CNPJ_CPF: return "conveyor_cnpj_cpf";
        case C_V_ADDRESS: return "conveyor_address";
        case C_V_COUNTY: return "conveyor_county";
        case C_V_UF_2: return "conveyor_uf_2";
        case C_V_STATE_REGISTRATION: return "conveyor_state_registration";
        case C_V_QUANTITY: return "conveyor_quantity";
        case C_V_SPECIES: return "conveyor_species";
        case C_V_MARK: return "conveyor_mark";
        case C_V_NUMBERING: return "conveyor_numering";
        case C_V_GROSS_WEIGHT: return "conveyor_gross_weight";
        case C_V_NET_WEIGHT: return "conveyor_net_weight";
        }
    }
    break;
    case PRODUCT_SERVICE_DATA:
    {
        return "product_service_data";
    }
    break;
    case ISSQN_CALCULATION:
    {
        switch(value)
        {
        case I_C_MUNICIPAL_REGISTRATION: return "issqn_municial_registration";
        case I_C_TOTAL_COST_SERVICES: return "issqn_total_cost_services";
        case I_C_ISSQN_CALCULATION_BASE: return "issqn_calculation_base";
        case I_C_COST_ISSQN: return "issqn_cost";
        }
    }
    break;
    case ADDITIONAL_DATA:
    {
        return "additional_data";
    }
    break;
    }

    return "";
}

QString Parser::GenerateJson(QMap<int, QList<sINVOICEDATA*>> map)
{
    QString json("");

    QString tmpStr;
    sINVOICEDATA* tmpInvoice = nullptr;

    json += "{\n";

    for(auto it = map.begin(); it != map.end(); ++it)
    {
        for(auto it2 = (*it).begin(); it2 != (*it).end(); ++it2)
        {
            tmpInvoice = (*it2);
            if(tmpInvoice != nullptr)
            {
                tmpStr = ConvertToJsonHeader(tmpInvoice->headerID, tmpInvoice->subHeaderID);
                if(tmpStr.isEmpty())
                    continue;

                json += "\t";
                json += "\"";
                json += tmpStr;
                json += "\"";
                json += ": ";

                if(tmpInvoice->value.isEmpty())
                {
                    json += "null";
                    json += ",\n";
                }
                else
                {
                    if(Utils::IsNumber(tmpInvoice->value))
                    {
                        json += tmpInvoice->value;
                        json += ",\n";
                    }
                    else
                    {
                        json += "\"";
                        json += tmpInvoice->value;
                        json += "\"";
                        json += ",\n";
                    }
                }
            }
        }
    }

    json.chop(2);
    json += "\n";
    json += "}";

    // TODO : Debug purpose.
    //printf("%s\n", json.toStdString().c_str());

    return json;
}

bool Parser::ConvertToJson()
{
    QString filename = "";

    if(_fileName.contains(".xml"))
        filename = _fileName.replace(".xml", "");
    else if(_fileName.contains(".pdf"))
        filename = _fileName.replace(".pdf", "");
    else
        filename = _fileName;

    filename += ".json";

    //printf("JSON will be saved in: %s\n", filename.toStdString().c_str());
    QFile file(filename);

    QString buffer = GenerateJson(_invoicesMap);
    if(buffer.isEmpty() || buffer.isNull())
    {
        printf("Failed to generate Json.\n");
        return false;
    }

    if (file.open(QIODevice::ReadWrite))
    {
        QTextStream stream(&file);
        stream << buffer << endl;
    }

    if(file.isOpen())
        file.close();

    //printf("%s", buffer.toStdString().c_str());
    return Utils::FileExists(filename);
}

///
/// \brief GetInvoiceData
/// \param pageMap
/// \return
///
bool Parser::GetInvoiceData()
{
    // List with possibles headers.
    QList<sTEXTDATA*> headers;
    QList<sTEXTDATA*> values;
    QMap<int, QList<sTEXTDATA*>> valuesMap;
    QList<sINVOICEHEADER*> invoiceHeaderList;

    QList<sTEXTDATA*> failed;

    // Temporary variables.
    sPAGE* page = nullptr;
    sTEXTDATA* textData = nullptr;
    sINVOICEDATA* invoiceData = nullptr;
    QString currentData;
    QString tmpStr;

    // Iterator of the page map.
    QMap<int, sPAGE*>::iterator itPage;

    // 0. Process each page.
    for(itPage = _pageMap->begin(); itPage != _pageMap->end(); ++itPage)
    {
        page = itPage.value();

        // 1. Let to get possibles headers.
        // We will discard texts that only have values.
        // Because it is more likely to get a possible header and try to read the value below the header.
        for(auto it = page->txtDataList.begin(); it != page->txtDataList.end(); ++it)
        {
            textData = (*it);

            // Process all the data obtained in the XML and add only
            // data that has a number of characters larger than numbers.
            if(Utils::CheckPossibleHeader(textData->text))
            {
                headers.push_back(textData);
            }

            values.push_back(textData);
        }

        // Check if we got possibles header, then let to process.
        if(headers.size() <= 0)
        {
            qDebug() << "Maybe error?";
            continue;
        }

        invoiceHeaderList = ProcessInvoiceHeader(values, page->width, page->height);

        // Check if we got possibles header.
        if(invoiceHeaderList.size() <= 0)
        {
            qDebug() << "Maybe error?";
            continue;
        }

        QList<sTEXTDATA*> tmpTxtList;
        QList<sINVOICEDATA*> tmpInvoiceList;
        sINVOICEHEADER* tmpHeader = nullptr;
        QRect tmpRect;
        //QList<QString> tmpStringList;
        int forMax = 0;

        for(int i = MAIN; i < MAX; i++)
        {
            tmpTxtList.clear();
            tmpHeader = nullptr;

            for(auto it = invoiceHeaderList.begin(); it != invoiceHeaderList.end(); ++it)
            {
                if((*it) && (*it)->header == i)
                {
                    tmpHeader = (*it);
                    break;
                }
            }

            if(tmpHeader == nullptr)
                continue;

            for(auto it = values.begin(); it != values.end(); ++it)
            {
                textData = (*it);

                tmpRect = QRect(QPoint(textData->left, textData->top),
                                QSize(textData->width, textData->height));

                if(tmpHeader->rect.intersects(tmpRect))
                {
                    tmpTxtList.push_back(textData);
                }
            }

            valuesMap.insert(i, tmpTxtList);
        }

        if(valuesMap.size() <= 0)
        {
            qDebug() << "Maybe error?";
            continue;
        }

        // Let to find values from headers.
        for(int i = MAIN; i < MAX; i++)
        {
            tmpInvoiceList.clear();

            if(i == PRODUCT_SERVICE_DATA)
                continue;

            forMax = GetMaxDataHeader(i);
            if(forMax == 0)
                continue;

            for(int k = 0; k < forMax; k++)
            {
                //tmpHeaderStr = _search->Convert((ConvertEnumToText(invoiceHeader->header)));
                //tmpTextData = _search->SearchText(tmpHeaderStr, possibleHeaders, AVERAGE_LEVENSTEIN_VALUE);
                //if(tmpTextData != nullptr)
                //{
                //    tmpRect = QRect(QPoint(0 /*tmpRect.left()*/, tmpRect.top()),
                //                    QSize(maxWidth, (tmpHeader->rect.top() - 1) - tmpRect.top()));
                //
                //    invoiceHeader->rect = tmpRect;
                //    invoiceHeaderList.push_back(invoiceHeader);
                //}

                tmpStr = _search->Convert(ConvertEnumToText(static_cast<eINVOICE_HEADER>(i), k));
                if(tmpStr.isNull() || tmpStr.isEmpty())
                {
                    qDebug() << "Failed to find str" << i << " " << k;
                    continue;
                }



                // TODO : Litwin
                //tmpStringList = ConvertEnumToText(static_cast<eINVOICE_HEADER>(i), k);
                //if(tmpStringList.length() <= 0)
                //{
                //    qDebug() << "Maybe error?";
                //    continue;
                //}
                //
                //tmpTxtList = valuesMap[i];
                //for(auto it = tmpStringList.begin(); it != tmpStringList.end(); ++it)
                //{
                //    tmpStr = (*it);
                //    textData = GetTextData(tmpStr, tmpTxtList);
                //
                //    if(textData != nullptr)
                //    {
                //        if(TryGetValue(textData, &tmpTxtList, &currentData, page->height, page->width))
                //        {
                //            invoiceData = new sINVOICEDATA();
                //            invoiceData->header = textData->text;
                //            invoiceData->value = currentData;
                //            invoiceData->headerID = i;
                //            invoiceData->subHeaderID = k;
                //
                //            tmpInvoiceList.push_back(invoiceData);
                //            break;
                //        }
                //        else
                //        {
                //            failed.push_back(textData);
                //        }
                //    }
                //}
            }

            _invoicesMap.insert(i, tmpInvoiceList);
        }
    }

    return (_invoicesMap.size() > 0) ? true : false;
}

///
/// \brief ReadInvoiceXML
/// \return
///
bool Parser::ReadInvoiceXML(QString fileName)
{
    _fileName = fileName;
    QString tmp = fileName.replace(".pdf", ".xml");

    printf("XML: %s\n", tmp.toStdString().c_str());

    QFile file(tmp);
    if(!file.open(QFile::ReadOnly | QFile::Text))
    {
        qDebug() << "Cannot read file" << file.errorString();
        return false;
    }

    _pageMap = new QMap<int, sPAGE*>();

    QXmlStreamReader reader(&file);

    // Temporary pointers.
    sFONTDESCRIPTION* fontDesc = nullptr;
    sTEXTDATA* txtData = nullptr;

    //printf("Let to read XML now.\n");

    //int count = 0;

    while(!reader.atEnd() && !reader.hasError())
    {
        //printf("[%d] - Reading XML.\n", count++);

        QXmlStreamReader::TokenType token = reader.readNext();
        if(token == QXmlStreamReader::StartDocument)
            continue;

        if(token == QXmlStreamReader::StartElement)
        {
            if(reader.name() == "pdf2xml")
                continue;

            if(reader.name() == "page")
            {
                int number = reader.attributes().value("number").toInt();
                QString position = reader.attributes().value("position").toString();
                int top = reader.attributes().value("top").toInt();
                int left = reader.attributes().value("left").toInt();
                int height = reader.attributes().value("height").toInt();
                int width = reader.attributes().value("width").toInt();

                sPAGE* pageData = new sPAGE();
                pageData->number = number;
                pageData->position = position;
                pageData->top = top;
                pageData->left = left;
                pageData->height = height;
                pageData->width = width;

                pageData->fontMap.clear();
                pageData->txtDataList.clear();

                reader.readNext();

                while(!(reader.tokenType() == QXmlStreamReader::EndElement && reader.name() == "page"))
                {
                    if(reader.tokenType() == QXmlStreamReader::StartElement)
                    {
                        if(reader.name() == "fontspec")
                        {
                            if(reader.tokenType() != QXmlStreamReader::StartElement)
                                continue;

                            int id = reader.attributes().value("id").toInt();
                            int size = reader.attributes().value("size").toInt();
                            QString family = reader.attributes().value("family").toString();
                            QString color = reader.attributes().value("color").toString();

                            fontDesc = new sFONTDESCRIPTION();
                            fontDesc->index = id;
                            fontDesc->size = size;
                            fontDesc->color = color;
                            fontDesc->family = family;

                            if(pageData->fontMap.contains(id))
                            {
                                qDebug() << "already has font added with same index!";
                                continue;
                            }

                            pageData->fontMap.insert(id, fontDesc);
                        }

                        if(reader.name() == "text")
                        {
                            if(reader.tokenType() != QXmlStreamReader::StartElement)
                                continue;

                            int topText = reader.attributes().value("top").toInt();
                            int leftText = reader.attributes().value("left").toInt();
                            int widthText = reader.attributes().value("width").toInt();
                            int heightText = reader.attributes().value("height").toInt();
                            int font = reader.attributes().value("font").toInt();

                            QString text = reader.readElementText(QXmlStreamReader::IncludeChildElements);
                            if(text == "")
                            {
                                qDebug() << "text is null.";
                                continue;
                            }

                            txtData = new sTEXTDATA();
                            txtData->top = topText;
                            txtData->left = leftText;
                            txtData->width = widthText;
                            txtData->height = heightText;
                            txtData->fontIndex = font;
                            txtData->text = text;

                            pageData->txtDataList.push_back(txtData);
                        }
                    }

                    reader.readNext();
                }

                // loaded everything from current page node,
                // now let to save in map.
                _pageMap->insert(static_cast<int>(pageData->number), pageData);
            }
        }
    }

    return (_pageMap && _pageMap->size() > 0) ? true : false;
}
