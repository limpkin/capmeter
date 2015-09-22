if (typeof capmeter == 'undefined') capmeter = {};
capmeter.graph = capmeter.graph || {};

capmeter.graph.unit = "V";
capmeter.graph.yLabel = "Current";
capmeter.graph.value = 0.0;
capmeter.graph.updateFrequency = 1000;

capmeter.graph.init = function() {
  var $graph = $(".graph-container");

  Highcharts.setOptions({
      global: {
          useUTC: false
      }
  }); 

  $graph.highcharts({
            chart: {
                type: 'spline',
                animation: Highcharts.svg,
                marginRight: 20,
                marignLeft: 0,
                events: {
                    load: function () {
                        var series = this.series[0];
                        setInterval(function () {
                            var x = (new Date()).getTime(), // current time
                                y = capmeter.graph.value;
                            series.addPoint([x, y], true, true);
                        }, capmeter.graph.updateFrequency);
                    }
                }
            },
            xAxis: {
                type: 'datetime',
                tickPixelInterval: 150,
            },
            yAxis: {
                title: {
                    text: capmeter.graph.yLabel
                },
                plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#ffffff'
                }]
            },
            tooltip: {
                formatter: function () {
                    return Highcharts.numberFormat(this.y, 2) + " " + capmeter.graph.unit;
                }
            },
            legend: {
                enabled: false
            },
            exporting: {
                enabled: false
            },
            series: [{
                name: 'Current',
                color: '#3ED1D6',
                data: (function () {
                    var data = [],
                        time = (new Date()).getTime(),
                        i;

                    for (i = -19; i <= 0; i += 1) {
                        data.push({
                            x: time + i * 1000,
                            y: 0
                        });
                    }
                    return data;
                }())
            }]
        });
}