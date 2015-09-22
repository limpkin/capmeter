if (typeof capmeter == 'undefined') capmeter = {};
capmeter.graph = capmeter.graph || {};

capmeter.graph.unit = "°F";
capmeter.graph.yLabel = "Temperature";
capmeter.graph.value = 0.0;

capmeter.graph.xLabels = ["0V", "1V", "2V", "3V", "4V", "5V"];
capmeter.graph.yValues = [1, 5, 2, 9, 5, 6];

capmeter.graph.changeXLabels = function(newLabels) {
  $(".graph-container").highcharts().xAxis[0].setCategories(newLabels);
  capmeter.graph.xLabels = newLabels;
}

capmeter.graph.changeYValues = function(newValues) {
  $(".graph-container").highcharts().series[0].setData(newValues);
}

capmeter.graph.init = function() {
  var $graph = $(".graph-container");

  Highcharts.setOptions({
      global: {
          useUTC: false
      }
  }); 

  $graph.highcharts({
            xAxis: {
                minPadding: 0.05,
                maxPadding: 0.05,
                categories: capmeter.graph.xLabels
            },

            series: [{
                data: capmeter.graph.yValues,
                color: "#3ED1D6"
            }],    
            chart: {
                type: 'spline',
                marginRight: 20,
                marignLeft: 0
            },
            yAxis: {
                title: {
                    text: capmeter.graph.yLabel + " " + capmeter.graph.unit
                },
                plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#888888'
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
            }
        });
}