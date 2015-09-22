if (typeof capmeter == 'undefined') capmeter = {};
capmeter.graph = capmeter.graph || {};

capmeter.graph.unit = " ";
capmeter.graph.yLabel = " ";
capmeter.graph.value = 0.0;

capmeter.graph.xLabels = ["", "", "", "", "", ""];
capmeter.graph.yValues = [0, 0, 0, 0, 0];

/* public functions */

capmeter.graph.changeXLabels = function(newLabels) {
  $(".graph-container").highcharts().xAxis[0].setCategories(newLabels);
  capmeter.graph.xLabels = newLabels;
}

capmeter.graph.changeYValues = function(newValues) {
  $(".graph-container").highcharts().series[0].setData(newValues);
  capmeter.graph.yValues = newValues; 
}

capmeter.graph.changeUnit = function(newUnit) {
  capmeter.graph.unit = newUnit;
  capmeter.graph.refreshYLabel();
}

capmeter.graph.changeYLabel = function(newYLabel) {
  capmeter.graph.yLabel = newYLabel;
  capmeter.graph.refreshYLabel();  
}

/* private functions */

capmeter.graph.refreshYLabel = function() {
  label = capmeter.graph.yLabel;
  unit = capmeter.graph.unit;
  $(".graph-container").highcharts().yAxis[0].axisTitle.attr({text:label + " " + unit});
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