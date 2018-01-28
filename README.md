# EnterpriseMan
Тестовое задание - программа для автоматизированного управления предпиятием 
*добавлена возможность иерархической организации отделов

usage: EnterpriseMan db_path

База данных:
divisions.csv - Отделы
duties.csv - Должностные обязанности
employees.csv - Сотрудники
position_duties.csv - Должность/обязанность
positions.csv - Должности

Структура таблиц:
divisions.csv:
  id - Идентификатор отдела
  div_id - Индентификатор подразделения, куда входит отдел. 
  name - Название отдела
  
duties.csv:
  id - идентификатор обязанности
  name - название 
  common - 1 - для всех, 0 - в соотвтствии с position_duties.csv
  
employees.csv:
  name - Имя сотрудника
  div_id - Отдел, в котором работает сотрудник
  pos_id - Идентификатор должности
 
position_duties.csv:
  pos_id - Идентификатор должности
  duty_id - идентификатор обязанности
  
positions.csv:
  id - Идентификатор должности
  name - Название

