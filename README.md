# Brute-force Mnemonic Old Electrum (V1)
## (Version 2.0.0)
## Генерация мнемонических фраз Electrum (V1) и соответствующих приватных ключей адресов m/0/x, m/1/x. Поиск адресов в базе.


## Файл config.cfg
* ***"folder_tables": "F:\\tables"***  - путь к папке с таблицами искомых адресов. Адреса в таблицах должны быть в формате hash160 и отсортированы программой https://github.com/Houzich/Convert-Addresses-To-Hash160-For-Brute-Force

* ***"number_of_generated_mnemonics": 18000000000000000000*** - Общее кол-во мнемоник которое мы хотим генерировать. Это введено для проверки скорости генерации или для сохранения результатов генерации в файлы. Если хотим бесконечно, то устанавливаем максимальное значение 18000000000000000000. 
* ***"num_child_addresses": 10*** - количество генерируемых адресов для каждого патча. От 1 до 10.</br></br>

* ***"path_m0_x": "yes"*** - генерировать ли адреса патча m/0/x? "yes" или "no".
* ***"path_m1_x": "yes"*** - генерировать ли адреса патча m/1/x? "yes" или "no".

* ***"chech_equal_bytes_in_adresses": "yes"*** - Проверять ли адереса на совпадение по байтам? "yes" или "no". Если "yes", то адреса будут проверяться на совпадение по байтам 
больше 8 байт. Смотри ниже в "Описание".
* ***"save_generation_result_in_file": "no"*** - Сохранять результат генерации в файл? "yes" или "no". Введено для проверки правильности генерации. Мнемоника и соответствующие ей адреса записываются в файл Save_Addresses.csv
Запись производится очень медленно. Так как преобразование hash160 в формат WIF производится на ЦПУ. При основной работе программы выбирать "no".</br></br>

* ***"static_words_generate_mnemonic": "suffocate threaten silently ? knee rhythm noise ? upon keep mud ?"*** - Какие слова генерировать? Можно задать слова мнемоники, которые будут постоянными. Генерироваться будут только те слова, которые указаны символом "?". К примеру, можно задать "suffocate threaten silently ? knee rhythm noise ? upon keep mud ?". Тогда генерироваться будут только 4, 8 и 12 слова.</br></br>

* ***"cuda_grid": 1024*** - настройка под видеокарту
* ***"cuda_block": 256*** - настройка под видеокарту
Кол-во генерируемых мнемоник за раунд равно cuda_grid*cuda_block


## Описание
При запуске программы, считываются настройки из файла config.cfg.
В консоли выводится надпись
> *Detected 3 CUDA Capable device(s)*

где число 3  - это количество найденных видеокарт NVIDIA.
Далее выводятся характеристики каждой карты:
> *Device 0: "NVIDIA GeForce GTX 1050 Ti"*</br>
> *...*</br>
> Device 1: "NVIDIA GeForce GTX 1050 Ti"</br>
> *...*</br>
> *Device 2: "NVIDIA GeForce GTX 1050 Ti"*</br>
> *Enter the number of the used video card:*</br>

Нужно ввести номер используемой карты.</br>

Начинается считывание и преобразование файлов таблиц с адресами:
> *PROCESSED 2168134 ROWS IN FILE F:\\tables\A0.csv*</br>
> *...* </br>
> *PROCESSED 1232455 ROWS IN FILE F:\\tables\A0.csv*</br>
> *...*</br>
> *PROCESSED 3455665 ROWS IN FILE F:\\tables\A0.csv*</br>
> *...*

Где 2168134 - это кол-во адресов в файле. Адреса в файле хранятся в 20 байтовом формате(hash160) в виде hex-строки. И отсортированы по возрастанию.

Далее выводится кол-во кошельков генерируемых за раунд. И начинается процесс генерации.
В ходе работы программы, постоянно обновляется надпись

> *GENERATE: 2,067 MNEMONICS/SEC AND 41,358 ADDRESSES/SEC | SCAN: 162.113215 GIGA ADDRESSES/SEC | ROUND: 5*

Кол-во мнемоник и кол-во адресов генерируемых за секунду и общее кол-во отсканированных адресов в таблицах. В данном случае, для каждого сгенерированного кошелька генерировалось 20 адресов. 10 адресов патча m/0/x и 10 адресов патча m/1/x

## Проверка на совпадение по байтам
Если при старте программы ввести
Если в файле config.cfg установить ***"chech_equal_bytes_in_adresses": "yes"***. То периодически на экране будут появляться надписи такого формата:
> *!!!FOUND IN ADDRESS(HASH160) (m/0/6) EQUAL 5 BYTES:: blossom window trouble everyday return use dot reflect sweat midnight cost made,14FtUvN1BHV4kto2t1V3pkAkZnLwrmat9f,14FtUvN14hVyyXJFDcwTkz5vkDyYWYBRCN,23B92CE0FF27BD38A6D7E1B8C8EED9D4F372E295,23B92CE0FF03C3AF18CF8A182D9B791CD260D8D0*

(*EQUAL 5 BYTES*) - количество совпавших байт. Мнемоника сгенерированного кошелька. Его адрес. Адрес в базе, который совпал по первым байтам с адресом мнемоники. И соответственно, их представление в 20-и байтовом формате hash160. Можно посчитать одинаковые байты и убедиться в этом.
Все эти адреса сохраняются в лог-файл Found_Bytes.csv.
В файле, строки хранятся в виде:
*EQUAL 5 BYTES,usual disagree error juice gap renew jacket toe circle goose tank prefer,15JMEsfkJSE1BJ3FjyMFZheAtpn7qLyHUs,15JMEsfjbTyHN6Wf9x2HA7XnqfVqdgD4kD,2F28782544E96EBEC694FFF37AC20FD2B6389ABD,2F2878254409A3E553D017294757CC2DDF4A2E99,Fri Feb  3 11:40:56 2023*

# Если нашли кошелек
В консоли появиться надписи:
> *!!!FOUND!!!*</br>
> *!!!FOUND!!!*</br>
> *!!!FOUND!!!*</br>
> *!!!FOUND!!!*</br>
> *!!!FOUND ADDRESS (m/0/8): suffocate threaten silently eventually knee rhythm noise remember upon keep mud suit, 1KDpfMLLWohA4E1iN2bv8qt3HZC957CYMU*</br>
> *!!!FOUND!!!*</br>
> *!!!FOUND!!!*</br>
> *!!!FOUND!!!*</br>
> *!!!FOUND!!!*</br>

Соответственно мнемоника и адрес который мы нашли. Эта информация добавиться в файл Found_Addresses.csv.
В файле строки хранятся в виде:
*suffocate threaten silently eventually knee rhythm noise remember upon keep mud suit,address path m/0/8,1KDpfMLLWohA4E1iN2bv8qt3HZC957CYMU,Tue May  9 20:26:14 2023*

## Файл BruteForceMnemonicOldV200.exe находится в папке exe

### ОБСУЖДЕНИЕ КОДА: https://t.me/BRUTE_FORCE_CRYPTO_WALLET

## If you want to support the project don't hesitate to donate.
**BTC** - bc1qqldn5lyk54rcvf5ndruh525v0qz8lf9yu5t9a5</br>
**ETH** - 0x1193901D25604F55f5fA93Be09F5203b4B6F265f