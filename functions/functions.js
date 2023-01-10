//
// functions placed in this section
//

// define function that swops sensor plots
function swapCanvases(){
    if(ctx.style.display=='block'){
        ctx.style.display='none';
        ctx2.style.display='block';
        btn.textContent = 'Plot Atmospheric Sensors';
        atmos = 0;
    }else{
        ctx.style.display='block';
        ctx2.style.display='none';
        btn.textContent = 'Plot Liquid Sensors';
        atmos = 1;
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
