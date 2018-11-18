#include "Search.h"

#include <QChar>

Search::Search()
{

}

QString Search::Convert(QString str)
{
    if(str.isEmpty() || str.isNull())
        return "";

    QString* data = new QString(str);

    // 1. convert to lower.
    ToLowerCase(data);

    // 2. remove all accents from string (case has accents).
    RemoveAccents(data);

    // 3. remove extra spaces on strings, example: "Str1  Str2" -> "Str1 Str2".
    RemoveExtraSpaces(data);

    return QString(*data);
}

bool Search::ToLowerCase(QString* str)
{
    if(str == nullptr)
        return false;

    *str = str->toLower();

    return true;
}

bool Search::RemoveAccents(QString* str)
{
    if(str == nullptr)
        return false;

    QChar c = QChar::Null;
    QString data = QString(*str);

    for(auto i = 0; i < data.length(); i++)
    {
        c = str->at(i);

        if(c.isNull()) continue;

        if(c == "â" || c == "ã" || c == "á" || c == "à" || c == "ä")
        {
            data[i] = 'a';
        }
        else if(c == "é" || c == "è" || c == "ê" || c == "ë")
        {
            data[i] = 'e';
        }
        else if(c == "í" || c == "ì" || c == "î" || c == "ï")
        {
            data[i] = 'i';
        }
        else if(c == "ó" || c == "ò" || c == "ô" || c == "õ" || c == "ö")
        {
            data[i] = 'o';
        }
        else if(c == "ú" || c == "ù" || c == "û" || c == "ü")
        {
            data[i] = 'u';
        }
        else if(c == "ç" || c == "ć")
        {
            data[i] = 'c';
        }
        else
        {
            continue;
        }
    }

    //str->clear();
    *str = data;

    return true;
}

bool Search::RemoveExtraSpaces(QString* str)
{
    if(str == nullptr)
        return false;

    QString data = "";
    std::unique_copy(str->begin(), str->end(),
                     std::back_insert_iterator<QString>(data),
                    [](QChar a, QChar b)
                    {
                        return a.isSpace() && b.isSpace();
                    });

    *str = data;

    return true;
}
