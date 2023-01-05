from datetime import datetime, timedelta
from flask import *
from functools import cmp_to_key
from pprint import pprint
from string import printable
import requests
import re


app = Flask(__name__)

def fetch():
    r = requests.get('https://robustudp.zoolab.org/')

    if not r.status_code == 200:
        print('[X] Status code is not 200')
        exit(1)

    pattern = re.compile(r'\<tr\>\n.*\<td\>(.*)\<\/td\>\n.*\<td\>(.*)\<\/td\>\n.*\<td\>(.*)\<\/td\>\n.*\<td\>(.*)\<\/td\>\n.*\<td\>(.*)\<\/td\>\n.*\<td\>(.*)\<\/td\>\n.*\<\/tr>')
    
    # html decode
    html = r.text.replace('&lt;', '<').replace('&gt;', '>').replace('&quot;', '"').replace('&amp;', '&')
    datas = re.findall(pattern, html)

    # sort by timestamp
    pre_data = sorted(datas, key=lambda x: datetime.strptime(x[0], '%Y-%m-%d %H:%M:%S'))

    '''
    {
        timestamp: {
            'team_name': [...],
            'success': [...],
            'corrupted': [...],
            'badsize': [...]
        }
    }
    '''
    global dataset
    dataset = {}
    team_name_set = set()
    last_status = {} # {team_name: (success, corrupted, badsize, elapsed)}
    for submit_time, team_name, n_corrupt, n_badsize, ratio, elapsed in pre_data:
        # limit team name width
        if len(team_name) > 15:
            team_name = team_name[:15] + '...'

        team_name_set.add(team_name)

        timestamp = datetime.strptime(submit_time, '%Y-%m-%d %H:%M:%S').timestamp()
        
        to_transform = {} # {teamname: (success, corrupted, badsize, elapsed)}

        dataset.setdefault(timestamp, {})
        dataset[timestamp].setdefault('team_name', [])
        dataset[timestamp].setdefault('success', [])
        dataset[timestamp].setdefault('corrupted', [])
        dataset[timestamp].setdefault('badsize', [])
        dataset[timestamp].setdefault('elapsed', [])

        # Ignore the submit that is not the highest success ratio
        last_team_status = last_status.get(team_name, (0, 0, 0, 0))
        if float(ratio) * 1000 >= last_team_status[0]:
            if float(ratio) * 1000 == last_team_status[0]:
                best_elapsed = min(float(elapsed[:-1]), last_team_status[3])
            else:
                best_elapsed = float(elapsed[:-1])
    
            to_transform[team_name] = (float(ratio) * 1000, int(n_corrupt), int(n_badsize), best_elapsed)
            last_status[team_name] = (float(ratio) * 1000, int(n_corrupt), int(n_badsize), best_elapsed)

        # fill other team's data according to last status
        for team_name in team_name_set:
            if team_name not in dataset[timestamp]['team_name']:
                last_success, last_corrupted, last_badsize, last_elapsed = last_status.get(team_name, (0, 0, 0, 0))
                to_transform[team_name] = (last_success, last_corrupted, last_badsize, last_elapsed)


        # Sort to_transform by success ratio and lowest elapsed time
        # {teamname: (success, corrupted, badsize, elapsed)}
        def _cmp(x, y):
            # success ratio
            if x[1][0] > y[1][0]:
                return 1
            elif x[1][0] < y[1][0]:
                return -1
            elif x[1][0] > 0 and x[1][0] == y[1][0]:
                if x[1][3] < y[1][3]:
                    return 1
                elif x[1][3] > y[1][3]:
                    return -1

            # corrupted + badsize
            if x[1][0] + x[1][1] + x[1][2] >  y[1][0] + y[1][1] + y[1][2]:
                return 1

            return -1
        
        to_transform = sorted(to_transform.items(), key=cmp_to_key(_cmp))[::-1]
        for team_name, data in to_transform:
            success, corrupted, badsize, elapsed = data
            dataset[timestamp]['team_name'].append(team_name)
            dataset[timestamp]['success'].append(success)
            dataset[timestamp]['corrupted'].append(corrupted)
            dataset[timestamp]['badsize'].append(badsize)
            dataset[timestamp]['elapsed'].append(round(elapsed, 5))

    print('[*] Fetch data successfully')

    return 'OK'


@app.route('/')
def index():
    global last_fetch
    if datetime.now() - last_fetch > timedelta(seconds=60):
        last_fetch = datetime.now()
        fetch()

    return render_template("index.html", timestamps=list(dataset.keys()), dataset=dataset)

if __name__ == "__main__":
    global last_fetch
    fetch()
    last_fetch = datetime.now()

    app.run(host='0.0.0.0', debug=False, port=8000)
