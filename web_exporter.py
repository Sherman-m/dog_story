# Импортируем три модуля:
# * json - для JSON
# * sys - для чтения из stdin
# * prometheus_client - для работы с Prometheus
# Последнему для краткости дадим имя prom

import prometheus_client as prom
import sys
import json

# Создаем четрые метрики
good_lines = prom.Counter('webexporter_good_lines', 'Good JSON records')
wrong_lines = prom.Counter('webexporter_wrong_lines', 'Wrond JSON records')

response_time = prom.Histogram(
    'webserver_request_duration', 
    'Response time',
    ['code', 'content_type'],
    buckets=(.001, .002, .005, .010, .020, .050, .100, .200, .500,
        float("inf")))

network_error = prom.Counter(
    'webserver_network_error',
    'Network error',
    ['where'])

# Определим точку входа
def my_main():
    # Читаем в цикле строку из стандартного ввода
    for line in sys.stdin:
        try:
            data = json.loads(line)

            # Ошибка, если из JSON прочитан не объект
            if not isinstance(data, dict):
                raise ValueError()

            # Регистрируем запрос
            if data['message'] == 'response sent':
                total_time_seconds = data['data']['response_time'] / 1000
                response_time.labels(
                        code=data['data']['code'],
                        content_type=data['data']['content_type']).observe(total_time_seconds)

                # Регистрируем успешно разобранную строку
                good_lines.inc()
            
            if data['message'] == 'error':
                network_error.labels(
                        where=data['data']['where']).inc()
                
        except (ValueError, KeyError):
            # Если пришли сюда, то при разборе JSON обнаружилась ошибка
            # Увеличим соответствующий счётчик
            wrong_lines.inc()

if __name__ == '__main__':
    prom.start_http_server(9200)
    my_main()
