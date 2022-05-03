import React, {useRef, useImperativeHandle, forwardRef} from 'react';
import ResizeObserver from 'rc-resize-observer';
import { Card } from 'antd';
import Graphin, { Behaviors, Components } from '@antv/graphin';

const { TreeCollapse } = Behaviors;
const { MiniMap,  Tooltip } = Components;
export default forwardRef((props, ref) => {
  useImperativeHandle(ref, () => ({
    Save,
    Refresh
  }));
  const graphinRef = useRef(null);
  const Save = () => {
    graphinRef.current.graph.downloadImage('graph', 'image/png' ,'#fff');
  }
  const Refresh = () => {
    graphinRef.current.graph.render();
  }
  
  return(
    <ResizeObserver onResize={({ width, height }) => {
      var graph = graphinRef.current.graph
      graph.changeSize(0.95*width, 0.95*height)
      graph.fitView()
    }}>
      <Graphin 
        ref = {graphinRef}
        data = {props.data}
        layout = {{
          type: 'compactBox',
          direction: 'TB',
          getId: function getId(d) {
            return d.id;
            },
            getHeight: function getHeight() {
                return 16;
            },
            getWidth: function getWidth() {
                return 16;
            },
            getVGap: function getVGap() {
                return 50;
            },
            getHGap: function getHGap() {
                return 50;
            },
        }}
        fitView
        fitCenter
        linkCenter
      >
          <MiniMap />
          <TreeCollapse trigger="click" />
          <Tooltip bindType="node" placement={"right"} hasArrow={true}
          style={
            {
              borderRadius:"20px",
              border:"5%",
              background:"#fff",
              opacity:"0.92",
              width:"180px"
            }
          }>
            {(value) => {
              if (value.model) {
                const { model } = value;
                return (
                  <Card title={model.label} size='small' style={{borderRadius:"20px"}}>
                    <li><b> - ctype:</b> {model.ctype} </li>
                    <li><b> - position:</b> {model.position} </li>
                    <li><b> - val:</b> {model.val} </li>
                  </Card>
                );
              }
              return null;
            }}
          </Tooltip>
      </Graphin>
    </ResizeObserver>
  )
});