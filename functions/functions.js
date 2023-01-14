//
// functions placed in this section
//

// define function that swops sensor plots
function swapCanvases(){
    if(ctx.style.display=='block'){
        ctx.style.display='none';
        ctx2.style.display='block';
        btn.textContent = 'Plot Atmospheric Sensors';
        atmos = false;
    }else{
        ctx.style.display='block';
        ctx2.style.display='none';
        btn.textContent = 'Plot Liquid Sensors';
        atmos = true;
    };
    sessionStorage.setItem("atmos", atmos);
}

// define the getdata function here
function getThingData(numVals) {
    // url for atmos sensor values
    const url = `https://api.thingspeak.com/channels/1627862/feeds.json?api_key=FOD7QX88PR1UBBLE&results=${numVals}`;

    // url for the liquid sensor values
    const url2 = `https://api.thingspeak.com/channels/1976934/feeds.json?api_key=M4Q2BX7R1ZWXV6P8&results=${numVals}`;

    // define arrays to contain IoT data
    let temperatures = [];
    let humidities = [];
    let humAlarms = [];
    let gasppms = [];
    let pressures = [];
    let gasAlarms = [];
    let timestamps =[];
    let levels = [];
    let turbidities = [];
    let levAlarms = [];
    let turAlarms = [];
    let timestamps2 =[];

    // fetch the atmos JSON data from the url
    fetch(url)
        .then(res => res.json())
        .then(data => {
            data.feeds.forEach(element => {
                temperatures.push(parseFloat(element.field1));
                humidities.push(parseFloat(element.field2));
                humAlarms.push(parseFloat(element.field3));
                gasppms.push(parseFloat(element.field5));
                pressures.push(parseFloat(element.field6));
                gasAlarms.push(parseFloat(element.field7));
                timestamps.push(element.created_at);
            });
        });

    // fetch the atmos JSON data from the url
    fetch(url2)
        .then(res => res.json())
        .then(data => {
            data.feeds.forEach(element => {
                levels.push(parseFloat(element.field1));
                turbidities.push(parseFloat(element.field2));
                levAlarms.push(parseFloat(element.field3));
                turAlarms.push(parseFloat(element.field4));
                timestamps2.push(element.created_at);
            });
        });
    
    return [temperatures, humidities, gasppms, pressures, levels, turbidities,  humAlarms,  gasAlarms, levAlarms, turAlarms, timestamps, timestamps2];
};

// get IoT data and plot
function plotData() {
    // get the number of values and what plot from session storage
    let numVals = parseInt(sessionStorage.getItem("numValues")) || 100;
    let atmos = sessionStorage.getItem("atmos") || true;
    sessionStorage.setItem("atmos", atmos);

    // can use getThingData function
    // get and import the data using getData fetch
    let [temperatures, humidities, gasppms, pressures, levels, turbidities, humAlarms, gasAlarms, levAlarms,  turAlarms,  timestamps,  timestamps2] =  getThingData(numVals);

    // set button element
    if (atmos) {
        btn.textContent = 'Plot Liquid Sensors';
        ctx.style.display='block';
        ctx2.style.display='none';
    } else {
        btn.textContent = 'Plot Atmospheric Sensors';
        ctx.style.display='none';
        ctx2.style.display='block';
    };

    // time stamps for atmospheric plots
    var xValues = timestamps;
    new Chart(ctx, {
        type: 'line',
        data: {
            labels: xValues,
            datasets: [{
                data: temperatures,
                borderColor: "green",
                fill: false,
                label: 'Temperature degC',
                yAxisID: 'TempHumid',
            },{
                data: humidities,
                borderColor: "blue",
                fill: false,
                label: 'Humidity %',
                yAxisID: 'TempHumid',
            },{
                data: gasppms,
                borderColor: "magenta",
                fill: false,
                label: 'Gas ppm',
                yAxisID: 'Gas',
            },{
                data: pressures,
                borderColor: "cyan",
                fill: false,
                label: 'Atm Bar',
                yAxisID: 'Pressure',
            },{
                data: levAlarms,
                borderColor: "red",
                fill: false,
                label: 'Humidity Alarm',
                yAxisID: 'Alarms',
            },{
                data: turAlarms,
                borderColor: "gold",
                fill: false,
                label: 'Gas Alarm',
                yAxisID: 'Alarms',
            }]
        },
        options: {
            plugins: {
                title: {
                    display: true,
                    text: 'ATMOSPHERIC SENSOR VALUES',
                }
            },
            legend: {display: true},
            layout: {
                    padding: 10
                    },
            scales: {
                xValues: {
                    type: 'time',
                    title: {
                    display: true,
                    text: 'Date'
                    },
                    time: {
                        unit: 'hour'
                    }
                },
                'TempHumid': {
                    position: 'left',
                    min: 0,
                    max: 100
                },
                'Gas': {
                    position: 'left'
                },
                'Pressure': {
                    position: 'left'
                },
                'Alarms': {
                    position: 'right',
                    min: 0,
                    max: 1
                }
            }
        },
    });

    // add a second one which may have a different timebase
    var xValues = timestamps2;
    new Chart(ctx2, {
        type: 'line',
        data: {
            labels: xValues,
            datasets: [{
                data: levels,
                borderColor: "green",
                fill: false,
                label: 'Level %',
                yAxisID: 'Level',
            },{
                data: turbidities,
                borderColor: "blue",
                fill: false,
                label: 'Tubidity',
                yAxisID: 'Turbidity',
            },{
                data: levAlarms,
                borderColor: "red",
                fill: false,
                label: 'Level Alarm',
                yAxisID: 'Alarms',
            },{
                data: turAlarms,
                borderColor: "gold",
                fill: false,
                label: 'Turbidity Alarm',
                yAxisID: 'Alarms',
            }]
        },
        options: {
            plugins: {
                title: {
                    display: true,
                    text: 'LIQUID SENSOR VALUES',
                }
            },
            legend: {display: true},
            layout: {
                    padding: 10
                    },
            scales: {
                xValues: {
                    type: 'time',
                    title: {
                    display: true,
                    text: 'Date'
                    },
                    time: {
                        unit: 'hour'
                    }
                },
                'Level': {
                    position: 'left',
                    min: 0,
                    max: 100
                },
                'Turbidity': {
                    position: 'left',
                    min: 0,
                    max: 1000
                },
                'Alarms': {
                    position: 'right',
                    min: 0,
                    max: 1
                }
            }
        },
    });
};