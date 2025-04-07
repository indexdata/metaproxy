# Metaproxy Helm chart

A Helm chart from this repository is published to the GitHub Container Registry (ghcr.io).
You can install the chart to your current cluster context/namespace with:

```
helm install metaproxy oci://ghcr.io/indexdata/charts/metaproxy --devel
```

Only development (aka master) version is published for now.

XML configuration for filters can be specified with the `metaproxy-filters-config` config map,
for example:

```bash
cat << 'EOF' | kubectl create configmap metaproxy-filters-config \
  --from-file=filters.xml=/dev/stdin \
  --dry-run=client \
  -o yaml > metaproxy-filters-config.yaml
<filter xmlns="http://indexdata.com/metaproxy" type="backend_test"/>
EOF
```

The `--dry-run` option only outputs the file but does not apply it on the cluster. To apply:

```bash
kubectl apply -f metaproxy-filters-config.yaml
```

then restart the deployment with:

```bash
kubectl rollout restart deployment metaproxy
```

You can check that `metaproxy` is running and that the filter has been applied with:

```bash
MP_HOST=$(kubectl get svc metaproxy -o jsonpath='{range .status.loadBalancer.ingress[0]}{@.ip}{@.hostname}{end}')
MP_PORT=$(kubectl get svc metaproxy -o jsonpath='{.spec.ports[0].port}')
yaz-client $MP_HOST:$MP_PORT
```

The chart uses the `LoadBalancer` service type at port `9000` by default,
this can be changed by passing the `service.type` and `service.frontendPort` values to the chart.

## Amazon EKS

If you're installing the chart on EKS and using the `LoadBalancer` service type,
you can expose the service to the public internet with:

```
--set "serviceAnnotations.service\.beta\.kubernetes\.io/aws-load-balancer-type=external" \
--set "serviceAnnotations.service\.beta\.kubernetes\.io/aws-load-balancer-nlb-target-type=instance" \
--set "serviceAnnotations.service\.beta\.kubernetes\.io/aws-load-balancer-scheme=internet-facing"
```
