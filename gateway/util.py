import requests

def notify(json=None):
    # https://ntfy.sh/lorawantestsboyanmihai
    requests.post('https://ntfy.sh/lorawantestsboyanmihai', json=json)